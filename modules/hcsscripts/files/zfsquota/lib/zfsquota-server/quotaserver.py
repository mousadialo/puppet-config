import ldap
import os
import re
import xmlrpclib
from scylla import xmlrpc

import subprocess_nointr

VALID_TYPES = set(['general', 'groups', 'hcs', 'people'])

LDAP_URL = 'ldap://hcs.harvard.edu:389'

# paths and arguments for zfs binaries
ZFS_CMD = ['/sbin/safezfs']
ZFS_GET_CMD = ['/sbin/zfs', 'get']

# quota-related zfs properties and corresponding display names
PROPERTIES = {'referenced':'used',
              'available':'free',
              'hcs:softquota':'limit',
              'refquota':'quota'}

def user_in_group(user, group):
    """
    Determine whether the given user is a member of the given LDAP group.
    NB: This function will not warn if the user or group doesn't exist.
    """
    # filter out characters that are invalid in an LDAP search string
    invalid = re.compile('[\\*()\0]')
    if invalid.search(user):
        raise ValueError('Invalid character in username')
    if invalid.search(group):
        raise ValueError('Invalid character in group')

    # connect to the server
    server = ldap.initialize(LDAP_URL)

    # the search filter to find unix group membership
    query = '(&(objectClass=posixGroup)(cn=%s)(memberUid=%s))' % (group, user)

    # search_s will return [] if there is no such record
    # we only grab the cn on success to avoid needless network traffic
    if server.search_s('dc=hcs,dc=harvard,dc=edu', ldap.SCOPE_SUBTREE, query,
                       ['cn']):
        return True
    else:
        return False

def require_group(ldap_group):
    """
    Decorator: Require that authuser is root or a member of given LDAP group.
    """
    def wrap(func):
        def newfunc(self, authuser, *args, **kwargs):
            if authuser == 'root' or user_in_group(authuser, ldap_group):
                print "require_group: '%s' is in '%s'" % (authuser, ldap_group)
                return func(self, authuser, *args, **kwargs)
            else:
                print "require_group: '%s' NOT IN '%s'" % (authuser,ldap_group)
                raise self._fail("Permission denied: not in " + ldap_group)

        newfunc.__doc__ = func.__doc__
        newfunc.__name__ = func.__name__
        return newfunc

    return wrap

def log_args(func):
    """
    Decorator: Print function arguments except the first (assumed to be self).

    This should be the outermost decorator in order to work properly, but be
    sure that the inner ones set __name__ appropriately.
    """
    def newfunc(*args, **kwargs):
        # a bit of a hack, but we don't want to see self
        str_args = ', '.join(map(str, args[1:]))
        if kwargs:
            str_args += ', ' + str(kwargs)
        print func.__name__ + '(' + str_args + ')'
        return func(*args, **kwargs)

    newfunc.__doc__ = func.__doc__
    newfunc.__name__ = func.__name__
    return newfunc

class Server(xmlrpc.ClientCertXMLRPC):
    allowNone = False

    def _make_path(self, type, name):
        if type not in VALID_TYPES:
            raise xmlrpclib.Fault(self.FAILURE, type + " is not a valid type")
        return "tank/home/%s/%s" % (type, name)

    def _fail(self, message):
        """Return a convenient xmlrpclib Fault exception to raise."""
        return xmlrpclib.Fault(self.FAILURE, message)

    def _run(self, cmd, print_commandline=True):
        """Run given commands in a subprocess. Does some crude error checks."""

        if print_commandline:
            print ' '.join(cmd)

        rc, out, err = subprocess_nointr.run(cmd)

        if rc > 1:
            raise xmlrpclib.Fault(self.FAILURE, "rc %d! -- args:" % rc, cmd)
            print "rc %d! -- args:" % rc, cmd

        if err:
            raise xmlrpclib.Fault(self.FAILURE, err.split('\n')[0])

        return rc, out, err

    def _run_zfs(self, cmd):
        """Run given zfs command in a subprocess."""
        return self._run(ZFS_CMD + cmd)

    def _run_zfs_get(self, cmd):
        """Run given zfs get command in a subprocess."""
        return self._run(ZFS_GET_CMD + cmd)

    @log_args
    def xmlrpc_get_quota(self, authuser, name, type):
        """
        Get ZFS quota information for specified user.
        Client must be root or in acctserfs to get quota of another user.
        """
        if authuser != name and authuser != 'root':
            if not user_in_group(authuser, 'acctserfs'):
                print "user '%s' NOT IN 'acctserfs'" % authuser
                raise self._fail("Permission denied: must be in acctserfs")

        # zfs get -H -p usedbydataset,available,hcs:softquota,refquota \
        #   tank/home/<type>/<name>

        rc, out, err = self._run_zfs_get(
            ['-H', '-p', ','.join(PROPERTIES), self._make_path(type, name)])

        # Read the output. (As follows, order variable):
        # tank/home/people/gdasher        usedbydataset    30600   -
        # tank/home/people/gdasher        available       3221194872      -
        # tank/home/people/gdasher        hcs:softquota   104857600       local
        # tank/home/people/gdasher        refquota   3221225472      local

        # Parse the result and return
        data = {}
        for line in out.strip().split('\n'):
            path, prop, value, source = line.split()[:4]
            data[PROPERTIES[prop]] = value

        return data

    @log_args
    @require_group('acctserfs')
    def xmlrpc_get_all_quotas(self, authuser):
        """
        Get ZFS quota information for all users.
        Client must be in acctserfs.

        NB: ZFS does not scale very well to large numbers of filesystems.
        This command can take a LONG time to complete, if it finishes at all.
        Because the server is currently synchronous, it will appear to be
        unresponsive until it finishes.
        """
        rc, out, err = self._run_zfs_get(
            ['-r', '-H', '-p', ','.join(PROPERTIES), 'tank/home'])

        results = {}

        for line in out.strip().split('\n'):
            path, prop, value, source = line.split()[:4]

            if path not in results:
                results[path] = {}
            results[path][PROPERTIES[prop]] = value

        return results

    @log_args
    @require_group('acctserfs')
    def xmlrpc_set_hardquota(self, authuser, name, type, bytes):
        """Set zfs refquota for given volume. Client must be in acctserfs."""
        bytes = int(bytes)

        self._run_zfs(
            ['set', 'refquota=%d' % bytes, self._make_path(type, name)])

        return True

    @log_args
    @require_group('acctserfs')
    def xmlrpc_set_softquota(self, authuser, name, type, bytes):
        """Set zfs softquota for given volume. Client must be in acctserfs."""
        bytes = int(bytes)

        self._run_zfs(
            ['set', 'hcs:softquota=%d' % bytes, self._make_path(type, name)])

        return True


    @log_args
    @require_group('acctserfs')
    def xmlrpc_create_filesystem(self, authuser, name, type, uid, gid,
                                 hardquota='', softquota=''):
        """
        Create a new ZFS filesystem at tank/home/$type/$username.
        Client must be in acctserfs.
        """
        fs = self._make_path(type, name)
        path = '/' + fs

        # make sure path doesn't already exist
        if os.path.exists(path):
            raise self._fail(path + " already exists!")

        # pfexec zfs create PATH
        self._run_zfs(['create', fs])

        if hardquota != '':
            self.xmlrpc_set_hardquota(authuser, name, type, hardquota)

        if softquota != '':
            self.xmlrpc_set_softquota(authuser, name, type, softquota)

        return True

    @log_args
    def xmlrpc_destroy_filesystem(self, authuser, name, type):
        """
        PERMANENTLY DESTROY a ZFS filesystem and all children.
        Client must be root. THERE IS NO UNDO.
        """
        if authuser != 'root':
            print "authuser '%s' is not root" % authuser
            raise self._fail('Permission denied: must be root')

        fs = self._make_path(type, name)

        self._run_zfs(['destroy', '-r', fs])

        return True
