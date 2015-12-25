from hcs import utils
import ldap, custom_exceptions, mailinglist
import settings.ldap, settings.user
from subprocess import Popen, PIPE
import logging, copy
import warnings

logger = logging.getLogger('driver')

# TODO: Verify LDAPS certificate. This just ignores it.
ldap.set_option(ldap.OPT_X_TLS_REQUIRE_CERT, ldap.OPT_X_TLS_NEVER)

def run(cmd, arg=None):
    """ Run a shell command and log output """
    if isinstance(cmd, str):
        cmd = [cmd]
    process = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    stdout, stderr = process.communicate(arg)
    log = ['Ran %s' % repr(cmd)]
    if stdout:
        log.append('\n-- Output: """\n%s"""' % stdout)
    if stderr:
        log.append('\n-- Stderr: """\n%s"""' % stderr)

    if process.returncode:
        logger.error(''.join(log))
    else:
        logger.info(''.join(log))
    return stdout

class LDAPDriverMetaclass(type):
    """ Metaclass for LDAPDriver """

class LDAPDriver(object):
    """ LDAP driver for storing user objects and data """
    __metaclass__ = LDAPDriverMetaclass
    # Format: pythonname:ldapname
    attr_mapping = {}
    con = ldap.initialize(settings.ldap.ldap_uri)
    try:
        con.bind_s(settings.ldap.ldap_directory_manager, settings.ldap.ldap_password)
    except ldap.SERVER_DOWN:
        # TODO FIXME: This is terrible design. Class definitions should NOT
        # have side effects. The ldap module might go to sleep or fork threads
        # while talking to the ldap server, which is NOT allowed during import
        # because it can cause python to deadlock.
        warnings.warn("LDAP server is down: %s" % settings.ldap.ldap_uri)
    base_dn_str = settings.ldap.ldap_base_dn
    base_dn = ldap.dn.str2dn(base_dn_str)
    filter = []

    def __init__(self, *args, **kwargs):
        self.original_modlist = {}
        super(LDAPDriver, self).__init__(*args, **kwargs)

    @classmethod
    def reconnect(self, username, password, uri = None):
        if not uri:
            uri = settings.ldap.ldap_uri
        self.con = ldap.initialize(settings.ldap.ldap_uri)
        self.con.bind_s(username, password)
        return self.con

    @classmethod
    def python2ldap(cls, attr):
        """ Change the attribute from its python name to its ldap name """
        return cls.attr_mapping.setdefault(attr, attr)

    @classmethod
    def ldap2python(cls, attr):
        """ Change the attribute from its ldap name to its python name """
        # TODO: make efficient
        for key, val in cls.attr_mapping.iteritems():
            if val == attr:
                return key
        cls.attr_mapping[attr] = attr
        return attr

    @classmethod
    def prepend_to_base(cls, dn):
        """
        Prepend a dn to the base_dn that is used as the base for
        searches
        """

        if isinstance(dn, str):
            dn = ldap.dn.str2dn(dn)
        cls.base_dn = dn + cls.base_dn
        # Allow for nice method chaining
        return cls.base_dn

    @classmethod
    def prepend_to_filter(cls, filter):
        """
        Prepend a filter to the filter used in searches.  Is a mutator.
        """
        # Make sure filter is wrapped in parens
        filter = cls._wrap_filter(filter)
        cls.filter.insert(0, filter)
        return cls.filter

    @classmethod
    def _wrap_filter(cls, filter):
        if isinstance(filter, str):
            filter = [cls._wrap_string(filter)]
        return filter

    @classmethod
    def _wrap_string(cls, filter):
        """ Puts parens around filter """
        if filter[0] == '(':
            if not filter[-1] == ')':
                raise ValueError, 'Filter %s seems malformed' % filter
        else:
            if filter[-1] == ')':
                raise ValueError, 'Filter %s seems malformed' % filter
            else:
                filter = '(%s)' % filter
        return filter

    @classmethod
    def prepended_to_filter(cls, filter, base_filter=None):
        """ Prepends filter to base_filter, and returns the result.  Does not
        mutate original objects. """
        if not base_filter:
            base_filter = cls.filter
        filter = cls._wrap_filter(filter)
        return filter + base_filter

    @classmethod
    def filter2str(cls, filter=None):
        """ Converts a filter to a string. """
        filter = [cls._wrap_string(entry) for entry in filter]
        if not filter:
            filter = cls.filter
        if len(filter) == 0:
            return None
        elif len(filter) == 1:
            return filter[0]
        else:
            # Concantenate filter elements and AND them together
            return '(&%s)' % ''.join(filter)

    @classmethod
    def find_ldapfilter(cls, ldapstr):
        filter = [ldapstr]
        filter = cls.prepended_to_filter(filter)
        filter_str = cls.filter2str(filter)
        logger.debug("Filter str is %s" % filter_str)
        if filter_str:
            result = cls.con.search_s(cls.base_dn_str, ldap.SCOPE_SUBTREE, filter_str)
        else:
            result = cls.con.search_s(cls.base_dn_str, ldap.SCOPE_SUBTREE, 'objectClass=*')
        try:
            logger.debug("First result: %r (total of %d results)" % (result[0], len(result)) )
        except IndexError:
            logger.debug( "No results found!" )
        return [cls.ldap_result2python(item) for item in result]

    @classmethod
    def find_all(cls, **kwargs):
        filter = ['(%s=%s)' % (cls.python2ldap(attr), value) for attr, value in kwargs.iteritems()]
        filter = cls.prepended_to_filter(filter)
        filter_str = cls.filter2str(filter)
        logger.debug("Filter str is %s" % filter_str)
        if filter_str:
            result = cls.con.search_s(cls.base_dn_str, ldap.SCOPE_SUBTREE, filter_str)
        else:
            result = cls.con.search_s(cls.base_dn_str, ldap.SCOPE_SUBTREE, 'objectClass=*')
        try:
            logger.debug( "First result: %r (total of %d results)" % (result[0], len(result)) )
        except IndexError:
            logger.debug( "No results found!" )
        return [cls.ldap_result2python(item) for item in result]

    @classmethod
    def ldap_result2python(cls, result):
        instance = cls()
        instance._load_ldap_result(result)
        instance.new_record = False
        return instance

    @classmethod
    def _create_find_by(cls, attr):
        def find_by_attr(cls, value):
            kwargs = {attr:value}
            result = cls.find_all(**kwargs)
            if len(result) == 0:
                raise custom_exceptions.ObjectNotFound, 'No objects with %s equal to %s found' % (attr, value)
            elif len(result) > 1:
                raise custom_exceptions.TooManyObjectsFound, 'Found %d objects with %s equal to %s, not a key' % (len(result), attr, value)
            return result[0]

        return classmethod(find_by_attr)


    def _modlist(self, attr = None):
        """
        Get an LDAP-style modlist of current attribute values.  If attr is
        specifed, then return modlist for only one attribute.
        """
        moddict = {}
        if attr is None:
            iterator = self.tracking
        else:
            iterator = [attr]

        for key in iterator:
            try:
                value = getattr(self, key)
            except AttributeError, e:
                logger.warn('Could not access attribute %s in object %s.  Error message: %s' % (key, self.id, e))
                continue
            if value is None:
                continue
            if isinstance(value, int):
                value = str(value)
            moddict[self.python2ldap(key)] = value
        return moddict

    def _original_modlist(self, attr = None):
        """ Get modlist for a single attr or for all attrs """
        moddict = {}
        if attr is None:
            for key, value in self.original_modlist.iteritems():
                moddict[self.python2ldap(key)] = value
        else:
            try:
                moddict[self.python2ldap(attr)] = self.original_modlist[attr]
            except KeyError:
                if attr != "shadowExpire":
                    logger.warn('No key %s found in model %s, defaulting to None' % (attr, self.id))
        return moddict

    def diff(self, attr = None):
        import ldap.modlist
        if self.new_record:
            return ldap.modlist.addModlist(self._modlist(attr))
        else:
            return ldap.modlist.modifyModlist(self._original_modlist(attr), self._modlist(attr))

    def getid(self):
        try:
            return self._id
        except AttributeError:
            return '%s=%s,%s' % (self.ldap_key, self.ldap_key_value, self.base_dn_str)

    def setid(self, value):
        self._id = value
    # TODO: clean this up
    id = property(getid, setid)

    def delete(self):
        if not self.new_record:
            self.con.delete_s(self.id)
        else:
            raise custom_exceptions.CustomError, "Cannot delete a record that hasn't even been created"

    def save(self, synchronous=True):
        modlist = self.diff()
        if self.new_record:
            id = self.id
            # A hack for while I can't figure out how to do callbacks better
            if not self.validate_required_attributes():
                raise 'This is a a hack: validation failed'
            logger.debug("Saving with id %r, modlist %r" % (id, modlist))
            if synchronous:
                self.con.add_s(id, modlist)
            else:
                self.con.add(id, modlist)
            self.new_record = False
        else:
            if modlist:
                if synchronous:
                    return self.con.modify_s(self.id, modlist)
                else:
                    return self.con.modify(self.id, modlist)
        return True

    def save_attribute(self, attr):
        """ Save a single attribute in LDAP """
        self.con.modify_s(self.id, self.diff(attr))

    def reload(self, attr=None):
        result = self.con.search_s(','.join(self.id.split(',')[1:]), ldap.SCOPE_SUBTREE, self.id.split(',')[0])
        if len(result) == 0:
            raise custom_exceptions.ObjectNotFound, 'No objects with dn equal to %s found' % (attr, self.id)
        elif len(result) > 1:
            raise custom_exceptions.TooManyObjectsFound, 'Found %d objects with dn %s, not a key' % (len(result), self.id)
        self._load_ldap_result(result[0], attr)
        self.new_record = False
        logger.debug('Reloaded object %s' % self.id)

    def _load_ldap_result(self, result, desired_attr = None):
        import ldap
        dn, original_attributes = result
        self.id = dn
        dn = ldap.dn.str2dn(dn)[0]
        for entry in dn:
            attr, value, integer = entry
            if attr == 'dc':
                continue
            attr = self.ldap2python(attr)
#            self.freeze(self.ldap2python(attr))
            if desired_attr and desired_attr != attr:
                continue
            setattr(self, attr, value)
            setattr(self, 'original_%s' % attr, copy.copy(value))

        attributes = {}
        for key, value in original_attributes.iteritems():
            key = self.ldap2python(key)
            if desired_attr and desired_attr != key:
                continue
            if len(value) == 1:
                attributes[key] = value[0]
            else:
                attributes[key] = value
        self.original_modlist = attributes

        for attr, value in attributes.iteritems():
            if desired_attr and desired_attr != attr:
                continue
            setattr(self, attr, value)
            setattr(self, 'original_%s' % attr, copy.copy(value))

    def validate_required_attributes(self):
        """
        Make sure required attributes exists and are set.
        """
        for attribute in self.neededForCreation:
            try:
                getattr(self, attribute)
            except AttributeError:
                return False
        return True

class LDAPUser(LDAPDriver):
    attr_mapping = {'username':'uid', 'uid':'uidNumber', 'gid':'gidNumber', 'home':'homeDirectory', 'shell':'loginShell', 'name':'gecos',
                    'access_list_emails':'uniqueMember'}
    ldap_key = 'uid'
    base_dn_str = 'ou=People,%s' % LDAPDriver.base_dn_str
    filter = ['objectClass=posixAccount']

    @property
    def ldap_key_value(self):
        return self.username

    @classmethod
    def username_exists(cls, username):
        """ See if a given username is taken """
        return len(cls.con.search_s(cls.base_dn_str, ldap.SCOPE_SUBTREE, '%s=%s' % (cls.python2ldap('username'), username)))

    def object_exists(cls, username):
        """ See if a given username is free to be used """
        return username_exists(cls, username) or mailinglist.isnameinuse(username)

    def next_uid(self):
        if self.type == 'member':
            start = 1001
            filter = '(|(&(uidNumber>=1000)(uidNumber<=1999))(uidNumber>=3000))'
        elif self.type == 'group':
            start = 2001
            filter = '(|(&(uidNumber>=2000)(uidNumber<=2999))(uidNumber>=4000))'
        elif self.type == 'general':
            start = 3001
            filter = '(|(&(uidNumber>=3000)(uidNumber<=3999))(uidNumber>=5000))'
        else:
            raise "Invalid Type"

        query = self.con.search_s(self.base_dn_str, ldap.SCOPE_SUBTREE, filter)
        claimed_uids = {}
        for result in query:
            uid = int(result[1]['uidNumber'][0])
            claimed_uids[uid] = 1
        next_uid = start
        while next_uid in claimed_uids:
            next_uid += 1
            if not next_uid % 1000:
                next_uid += 1000
            
        return next_uid

    def save(self):
        success = super(LDAPUser, self).save()
        if success == False:
            return success

        # If I just set the password...
        # TODO: make this detect password changes, rather than just being not none
        if self.password is not None:
            logger.debug("Setting password for %s" % self.username)
            if not self.password:
                logger.info('Password set to empty value; randomly generating password and expiring password')
                # For deactivated accounts.  Possibly also for setting the password and emailing it to people.
                self.password = utils.randstring(10)
                self.setstate('expire-password', True)
            # run(['passwd', self.username], '%s\n%s\n' % (self.password, self.password))
            self.con.passwd_s(self.id, None, self.password)
        self.reload(attr='shadowLastChange')
        if self.state('expire-password'):
            self.do_expire_password()
        elif self.state('unexpire-password'):
            self.do_unexpire_password()
        return True

class LDAPGroup(LDAPDriver):
    attr_mapping = {'name':'cn', 'members':'memberUid', 'gid':'gidNumber'}
    ldap_key = 'cn'
    base_dn_str = 'ou=Group,%s' % LDAPDriver.base_dn_str

    @property
    def ldap_key_value(self):
        return self.name
