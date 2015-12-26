from hcs import utils
import custom_exceptions, model, driver, observers
import re, os, shutil, datetime
import logging

logger = logging.getLogger('models')

class User(model.Model, driver.LDAPUser):
    """
    Model for an HCS user
    """
    import hcs.info as info
    valid_types = ['member', 'group', 'general', 'hcs']

    def save(self):
        return super(User, self).save()

    def __str__(self):
        try:
            return "<User username='%s'>" % self.username
        except AttributeError:
            return "<User username=''>"

    def __repr__(self):
        return str(self)

    @property
    def quota(self):
        quotadict = {'member':'2G', 'general':'1G', 'group':'2G'}
        return quotadict[self.type]

    def gettype(self):
        """Allow someone to explicitly set type; otherwise infer from gid."""
        try:
            type = self._type
        except AttributeError:
            # HACK: to stop infinite recursion.
            if hasattr(self, '_checking_type') and self._checking_type:
                self.gid = 2000
            self._checking_type = True
            if self.gid == 1000:
                type = 'member'
            elif self.gid == 2000:
                type = 'group'
            elif self.gid == 3000:
                type = 'general'
            elif self.gid == 1200:
                type = 'hcs'
            else:
                raise custom_exceptions.InvalidGroup, 'Unrecognized type %s' % self.username

        return type

    def settype(self, type):
        if type not in self.valid_types:
            raise custom_exceptions.CustomError, 'Unrecognized type %s' % self.username
        self._type = type
    # TODO: install python 2.6 and make this cleaner
    type = property(gettype, settype)

    @property
    def email(self):
        return '%s@hcs.harvard.edu' % self.username

    def default_home(self):
        if self.type == 'member':
            return '/home/people/%s' % self.username
        elif self.type == 'group':
            return '/home/groups/%s' % self.username
        elif self.type == 'general':
            return '/home/general/%s' % self.username
        elif self.type == 'hcs':
            return '/home/hcs/%s' % self.username
        else:
            raise custom_exceptions.InvalidGroup, 'Unrecognized type %s' % self.username

    def default_gid(self):
        if self.type == 'member':
            return 1000
        elif self.type == 'group':
            return 2000
        elif self.type == 'general':
            return 3000
        elif self.type == 'hcs':
            return 1200
        else:
            raise custom_exceptions.InvalidGroup, 'Unrecognized type %s' % self.username

    def default_uid(self):
        self.uid = self.next_uid()
        return self.uid

    def config_file(self, name):
        """ Get the config file associated with name """
        if name == 'authorized_keys':
            return os.path.join(self.home, '.ssh', name)
        else:
            return os.path.join(self.home, '.%s' % name)

    def _get_access_list(self):
        if not self.type == 'group':
            raise AttributeError, 'Only groups have access lists'
        if not hasattr(self, '_access_list'):
            self._access_list = AccessList(user=self)
        return self._access_list

    def _set_access_list(self, value):
        """ Cheating method to allow the access list object to deal with value internally.  Not working right now. """
        raise NotImplemented
        self.access_list[:] = value
    access_list = property(_get_access_list, _set_access_list)

    def _get_outside_email(self):
        try:
            return self._outside_email
        except AttributeError:
            return None

    def _set_outside_email(self, value):
        self._outside_email = value
        self.setstate('set_outside_email', True)
    outside_email = property(_get_outside_email, _set_outside_email)

    def set_random_password(self):
        self.setstate('set_random_password', True)
        self.password_confirmation = self.password = utils.randstring(6)
        return self.password

    def take_file(self, file, mode=None):
        """Set perms and owner for a file."""
        os.chown(file, self.uid, self.gid)
        if mode is not None:
            os.chmod(file, mode)
            logger.debug("Chmoded %s to %o, chowned %d:%d" %
                         (file, mode, self.uid, self.gid))
        else:
            logger.debug("Chowned %s to %d:%d" %
                         (file, self.uid, self.gid))

    def take_path(self, path, mode=None):
        """Recursively set perms and owner of given file path."""
        self.take_file(path, mode)
        for root, dirs, files in os.walk(path):
            for name in dirs + files:
                self.take_file(os.path.join(root, name), mode)

    def create_initial_files(self):
        # Touch the k5login
        open(self.config_file('k5login'), 'a').close()
        shosts = open(self.config_file('shosts'), 'a')
        shosts.write(self.default_shosts())
        shosts.close()
        # Only members have .forward created, and only if they have set an outside email
        # We touch and chmod it for others
        if self.type == 'member' and self.state('set_outside_email'):
            forward = open(self.config_file('forward'), 'a')
            emails =  ['\%s' % self.username, self.outside_email]
            forward.write(', '.join(emails))
            forward.close()
            self.take_file(self.config_file('forward'), 0600)

        if self.type == 'group':
            self.access_list.save()

        self.take_file(self.config_file('k5login'), 0600)
        self.take_file(self.config_file('shosts'), 0600)

        skeldir = '/etc/skel'
        for filename in os.listdir(skeldir):
            src = os.path.join(skeldir, filename)
            dst = os.path.join(self.home, filename)
            if os.path.isdir(src):
                shutil.copytree(src, dst)
                self.take_path(dst)
            else:
                shutil.copy2(src, dst)
                self.take_file(dst)

    def default_shosts(self):
        return 'cato.hcs.harvard.edu %s\ncaesar.hcs.harvard.edu %s\n' % (self.username, self.username)

    def default_cn(self):
        return self.name

    def default_objectClass(self):
        if self.type == 'group':
            return ['account', 'posixAccount', 'shadowAccount', 'top', 'groupofuniquenames']
        else:
            return ['account', 'posixAccount', 'shadowAccount', 'top']

    def defaultShadowLastChange(self):
        if self.password:
            return -1

    def _get_access_list_emails(self):
        return self.access_list.emails

    def _set_access_list_emails(self, value):
        self._access_list_emails = value

    def default_access_list_emails(self):
        return []

    access_list_emails = property(_get_access_list_emails, _set_access_list_emails)

    def expire_password(self):
        self.setstate('expire-password', True)
        self.shadowLastChange = 0

    def unexpire_password(self):
        self.setstate('unexpire-password', True)
        self.shadowLastChange = -1

    def do_expire_password(self):
        self.update_attribute('shadowLastChange', 0)

    def do_unexpire_password(self):
        self.update_attribute('shadowLastChange', -1)

    def _get_expiry_date(self):
        """Gets account expiry date as datetime.date or None"""
        try:
            expiry = self.shadowExpire
        except AttributeError:
            return None

        # Any value under 1 is considered non-expiry
        if expiry < 1:
            return None

        try:
            # shadowExpire is days since Jan 1, 1970
            return datetime.date.fromordinal(expiry + datetime.date(1970,1,1).toordinal())
        except ValueError:
            logger.error("Value of shadowExpire in LDAP for user %s is invalid" % self.username)
            return None

    def _set_expiry_date(self, value):
        """Sets the account expiry date. value can be datetime.date or None"""
        if value is None:
            self.update_attribute('shadowExpire', -1)
        else:
            try:
                expiry = value.toordinal() - datetime.date(1970,1,1).toordinal()
                self.update_attribute('shadowExpire', expiry)
            except ValueError:
                raise custom_exceptions.CustomError, 'Invalid expiry date %s' % value
    expiry_date = property(_get_expiry_date, _set_expiry_date)

    username = model.stringField(default=model.Model._require_assignment)
    cn = model.stringField(default=default_cn)
    name = model.stringField()
    shell = model.stringField(default='/bin/bash')
    home = model.stringField(default=default_home)
    ou = model.stringField(default='People')
    uid = model.integerField(default=default_uid)
    gid = model.integerField(default=default_gid)

    userPassword = model.stringField()
    shadowLastChange = model.integerField(default=defaultShadowLastChange)
    shadowMin = model.integerField()
    shadowMax = model.integerField()
    shadowWarning = model.integerField()
    shadowInactive = model.integerField()
    shadowExpire = model.integerField()
    shadowFlag = model.integerField()

    password = model.stringField()
    password_confirmation = model.stringField()
    objectClass = model.listField(default=default_objectClass)

    @property
    def neededForCreation(self):
        if self.type in ['member', 'general']:
            return model.attributesForCreation('username', 'uid', 'gid', 'home', 'shell', 'objectClass', 'home', 'cn')
        else:
            return model.attributesForCreation('username', 'uid', 'gid', 'home', 'shell', 'objectClass', 'home', 'cn', 'access_list_emails')

    @property
    def tracking(self):
        if self.type in ['member', 'general']:
            return model.persistentAttributes('username', 'uid', 'gid', 'home', 'shell', 'objectClass',
                                              'home', 'cn', 'shadowMin', 'shadowMax', 'shadowWarning', 'shadowInactive',
                                              'shadowExpire', 'shadowFlag', 'name', 'shadowLastChange', 'userPassword')
        else:
            return model.persistentAttributes('username', 'uid', 'gid', 'home', 'shell', 'objectClass',
                                              'home', 'cn', 'shadowMin', 'shadowMax', 'shadowWarning', 'shadowInactive',
                                              'shadowExpire', 'shadowFlag', 'name', 'shadowLastChange', 'userPassword',
                                              'access_list_emails')


User.add_observer(observers.UserObserver)

class UnknownType(Exception):
    pass

class AccessList(object):
    # Enum
    FAS_USER = 0
    HCS_USER = 1
    SSH_KEY = 2
    EMAIL = 3

    at_re = re.compile(r'^(.*)@(.*)$')
    hcs_shosts_re = re.compile(r'^.*\.hcs\.harvard\.edu ([\w]*)$')
    email_re = re.compile(r'^[a-z0-9\-\.+_]+@[a-z0-9\-\.+_]+\.[a-z0-9\-\.+_]+$', re.IGNORECASE)
    fas_re = re.compile(r'^[a-z0-9\-\.+_]+$', re.IGNORECASE)
    hcs_re = re.compile(r'^[a-z0-9\.+_]+$', re.IGNORECASE)
    key_re = re.compile(r'^ssh-[a-zA-Z]+[ ]+[0-9a-zA-Z\+/=\\/]{60,}.*$', re.IGNORECASE)

    def __init__(self, user=None, fas_users=None, emails=None, hcs_users=None, ssh_keys=None, write=True):
        self.user = user
        self.write = write
        self._emails = emails or set()
        self._hcs_users = hcs_users or set()
        self._ssh_keys = ssh_keys or set()

        try:
            self.emails = user._access_list_emails
        except AttributeError:
            self.emails = []


        self.load_auxiliary()

    # Checks that (email|hcs_user|ssh_key) is of a valid form
    def validate(self, item, type):
        if type == 'email':
            return self.email_re.search(item)
        elif type == 'hcs_user':
            return self.hcs_re.search(item)
        elif type == 'ssh_key':
            return self.key_re.search(item)
        else:
            raise UnknownType, 'Unknown type %s' % type

    def __delitem__(self, i):
        if i < 0:
            return

        # Remove the ith elt of a set
        def _delete_from_set(set, i):
            set.remove(self.set2list(set)[i])

        for sets in [self._emails, self._hcs_users, self._ssh_keys]:
            if i < len(sets):
                _delete_from_set(sets, i)
                return
            else:
                i -= len(sets)

    # Add item of type type.  Type should be address|hcs_user|ssh_key
    def _add(self, item, type):
        item = self.canonicalize(item, type)
        if type == 'email':
            if self.validate(item, type):
                self._emails.add(item)
                return True
        elif type == 'hcs_user':
            if self.validate(item, type):
                self._hcs_users.add(item)
                return True
        elif type == 'ssh_key':
            if self.validate(item, type):
                self._ssh_keys.add(item)
                return True
        else:
            raise UnknownType, 'Unknown type %s' % type
        # If made it here, did not successfully add
        return False

    def _delete(self, item, type):
        item = self.canonicalize(item, type)
        if type == 'email':
            self._emails.remove(item)
        elif type == 'hcs_user':
            self._hcs_users.remove(item)
        elif type == 'ssh_key':
            self._ssh_keys.remove(item)
        else:
            raise UnknownType, 'Unknown type %s' % type

    def add_email(self, email):
        return self._add(email, 'email')

    def add_hcs_user(self, hcs_user):
        return self._add(hcs_user, 'hcs_user')

    def add_ssh_key(self, ssh_key):
        return self._add(ssh_key, 'ssh_key')

    def set2list(self, set):
        a = list(set)
        a.sort()
        return a

    def to_list(self):
        """Convert access list to list"""
        return self.emails + self.hcs_emails + self.ssh_keys

    def to_tokentype_list(self):
        """Convert access list to token type list"""
        return [(e, self.EMAIL) for e in self.emails] \
            + [(e, self.HCS_USER) for e in self.hcs_usernames] \
            + [(e, self.SSH_KEY) for e in self.ssh_keys]

    def __str__(self):
        return str(self.to_list())

    def __repr__(self):
        return repr(self.to_list())

    def __len__(self):
        return len(self.to_list())

    def load_auxiliary(self):
        """Loads HCS and key users from flat files"""
        hcs_re = re.compile(r'caesar\.hcs\.harvard\.edu', re.IGNORECASE)
        me_re = re.compile(r'\.hcs\.harvard\.edu %s$' % self.user.username, re.IGNORECASE)
        try:
            self.hcs_usernames = [self.canonicalize(line.strip(), 'hcs_user') for line in open(self.user.config_file('shosts')) if hcs_re.search(line) and not me_re.search(line)]
        except IOError:
            pass

        try:
            self._ssh_keys = set([self.canonicalize(line.strip(), 'ssh_key') for line in open(self.user.config_file('authorized_keys'))])
        except IOError:
            pass

    def load_fas_from_file(self):
        logger.warn('load_fas_from_file: fas_user is deprecated')
        fas_re = re.compile(r'^.+@fas\.harvard\.edu$', re.IGNORECASE)
        try:
            f = open(self.user.config_file('k5login'), 'r')
        except IOError:
            pass
        else:
            for line in f:
                if fas_re.search(line):
                    self.add_email(line.strip().lower())

    def __iter__(self):
        return iter(self.to_list())

    def canonicalize(self, item, type):
        item = item.strip()
        if type == 'email':
            # canonical email is the full email
            pass
        elif type == 'fas_user':
            logger.info('canonicalize: fas_user is deprecated')
            match = self.at_re.search(item)
            if match:
                item = match.group(1)
        elif type == 'hcs_user':
            match = self.at_re.search(item) or self.hcs_shosts_re.search(item)
            if match:
                item = match.group(1)
        elif type == 'ssh_key':
            pass # Currently, can't do anything to canonicalize ssh key
        else:
            raise UnknownType, 'Unknown type %s' % type
        return item

    def save(self):
        """
        Write access list to file.  Saving to LDAP happens as part of
        a standard user save.  The shosts writing is only for
        compatibility with current access script and will be removed
        soon.
        """

        if self.write:
            k5login = open(self.user.config_file('k5login'), 'w')
            for user in self.fas_usernames:
                k5login.write('%s@FAS.HARVARD.EDU\n' % user)
            shosts = open(self.user.config_file('shosts'), 'w')
            shosts.write(self.user.default_shosts())
            for user in self.hcs_usernames:
                shosts.write('cato.hcs.harvard.edu %s\n' % user)
                shosts.write('caesar.hcs.harvard.edu %s\n' % user)
            authkeys_file = self.user.config_file('authorized_keys')
            dotssh = os.path.dirname(authkeys_file)
            if not os.path.exists(dotssh):
                os.mkdir(dotssh)
                self.user.take_file(dotssh, 0700)
            authkeys = open(authkeys_file, 'w')
            for key in self.ssh_keys:
                authkeys.write('%s\n' % key)

            # Restore correct perms
            self.user.take_file(self.user.config_file('k5login'))
            self.user.take_file(self.user.config_file('shosts'))
            self.user.take_file(self.user.config_file('authorized_keys'))
        return True

    # fas usernames can no longer be set
    @property
    def fas_usernames(self):
        return self.set2list(self.canonicalize(i, 'fas_user') for i in self._emails if i.endswith('@fas.harvard.edu'))

    # TODO: upgrade python
    def _get_emails(self):
        return self.set2list(self._emails)

    def _set_emails(self, value):
        """
        Remove all current email addresses from the access list and then add
        the new ones.
        """
        if isinstance(value, str):
            value = [value]
        self._emails = set([self.canonicalize(item, 'email') for item in value])

    emails = property(_get_emails, _set_emails)

    # TODO: upgrade python
    def _get_hcs_usernames(self):
        return self.set2list(self._hcs_users)

    def _set_hcs_usernames(self, value):
        """ Remove all current HCS usernames from the access list and then add the new ones """
        if isinstance(value, str):
            value = [value]
        self._hcs_users = set([self.canonicalize(item, 'hcs_user') for item in value])

    hcs_usernames = property(_get_hcs_usernames, _set_hcs_usernames)

    def _get_ssh_keys(self):
        return self.set2list(self._ssh_keys)

    def _set_ssh_keys(self, value):
        if isinstance(value, str):
            value = [value]
        self._ssh_keys = set([self.canonicalize(item, 'ssh_key') for item in value])

    ssh_keys = property(_get_ssh_keys, _set_ssh_keys)

    @property
    def fas_emails(self):
        return [email for email in self.set2list(self._emails) if email.endswith('@fas.harvard.edu')]

    @property
    def hcs_emails(self):
        return ['%s@hcs.harvard.edu' % email for email in self.set2list(self._hcs_users)]

class Group(driver.LDAPGroup, model.Model):
    """ Represents a UNIX group """

    name = model.stringField(default=model.Model._require_assignment)
    gid = model.integerField(default=model.Model._require_assignment)
    objectClass = model.listField(default=['posixGroup', 'top'])
    members = model.listField()

    # TODO: there is also a userPassword attr... what does this do?
    neededForCreation = model.attributesForCreation('name', 'objectClass', 'gid', 'members')
    tracking = model.persistentAttributes('name', 'objectClass', 'gid', 'members')

