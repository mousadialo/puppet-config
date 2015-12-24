import hcs.settings
import logging, os
from hcs.mailers import Mailer
from email.Utils import formatdate
logger = logging.getLogger('mailers')

class UserMailer(Mailer):
    def _setup(self, user, send_to=None, cc=None, access_list=True, outside=True, send_from=None):
        """
        Set up an email.  user can be either a model or it can be an
        email address (or a list of email addresses.  Alternatively,
        one can specify a send_to option.
        """
        if send_to is None:
            if isinstance(user, str):
                send_to = [user]
            elif isinstance(user, list):
                send_to = user
            else:
                send_to = user.email

                # Only add a CC if they are going for default send_to
                if cc is None:
                    if user.type == 'group' and access_list:
                        cc = user.access_list.emails
                    elif user.type == 'member' and user.state('set_outside_email') and outside:
                        cc = [user.outside_email]

        self.send_from = send_from or hcs.settings.mail.send_from
        self.date = formatdate(localtime=True)
        self.user = user
        self.send_to = send_to
        self.cc = cc

    def creation(self, user, access_list=True):
        """ Email to send upon creation """
        self._setup(user, access_list=access_list)
        self.subject = 'Your new HCS account: %s@hcs' % user.username

    def www_info(self, user, access_list=False):
        """ Another email to send upon creation """
        self._setup(user, access_list=access_list)
        self.subject = 'Your HCS web hosting'

    def user_info(self, user, access_list=False):
        """ Yet another email to send upon creation """
        self._setup(user, access_list=access_list)
        self.subject = 'Info about your HCS account'

    def member_welcome(self, user, outside=True):
        """ Email for members """
        self._setup(user, outside=outside)
        self.subject = 'Welcome to HCS'

    def acctserv_notification(self, user, action, **kwargs):
        self.acctserf = kwargs.setdefault('sendfrom', '')
        # Hack for multithreaded
        if not self.acctserf:
            self.acctserf = os.getlogin()
        self._setup(user, send_to='hcs-systems@hcs.harvard.edu',
                    send_from='%s@hcs.harvard.edu' % self.acctserf)
        self.to_real = ['hcs-systems-bots@hcs.harvard.edu']
        if action == 'creation':
            display_action = 'Account creation for %s' % user.username
        elif action == 'mysql_database_creation':
            if not isinstance(user, str):
                display_action = 'Created mysql database for %s' % user.username
            else:
                display_action = 'Created mysql database %s' % kwargs['database']
        elif action == 'vhosts_admin':
            try:
                username = user.username
            except AttributeError:
                username = user
            # Someone editing vhosts
            display_action = 'Vhost administration for %s' % username
        elif action == 'vhosts_pull':
            # Vhosts were pulled
            display_action = 'Vhosts auto-pull'
        else:
            raise ValueError, 'Unsupported action %s' % action

        self.subject = 'AcctServ Action: %s' % display_action
        self.action = action
        self.display_action = display_action
        self.log = hcs.settings.log.getlog()

class MysqlMailer(Mailer):
    def _setup(self, user, send_to=None, cc=None):
        if not send_to:
            send_to = user.email

        self.send_from = hcs.settings.mail.send_from
        self.date = formatdate(localtime=True)
        self.user = user
        self.send_to = send_to
        self.cc = cc

    def creation(self, user, username, password, database):
        """ Email to send upon creation """
        self._setup(user)
        self.username = username
        self.password = password
        self.database = database
        self.subject = 'Your new MySQL database %s' % database

    def newpassword(self, user, username, password):
        """ Email to send when password is changed """
        self._setup(user)
        self.username = username
        self.password = password
        self.subject = 'Your MySQL database password has been changed'

class OneoffMailer(Mailer):
    def _setup(self, send_to, send_from=None, cc=None):
        self.send_to = send_to
        self.send_from = send_from or hcs.settings.mail.send_from
        if cc:
            self.cc = cc
        self.date = formatdate(localtime=True)

    def sibs_welcome(self, user, littlesibs, mode, littlesib=None, send_from=None):
        """
        Send email to either user or to.  mode should be either 'big' or
        'little', indicating who should be receiving the email.  Sibs
        should be the list of little sibs associated with user.
        """
        self.template_name = 'none.tmpl'
        self.big_sib_first = user.name.split(' ')[0]
        if mode == 'big':
            send_to = user.email
            self.name = self.big_sib_first
        else:
            send_to = littlesib['email']
            self.name = littlesib['name'].split(' ')[0]
        self._setup(send_to, send_from)
        self.user = user
        self.littlesibs = littlesibs
        self.mode = mode
        self.subject = 'HCS Big Sibs/Little Sibs Assignment'
        self.wrap = True
        self.littlesib_rep = '\n'.join(['%s (%s)' % (littlesib['name'], littlesib['email']) for littlesib in littlesibs])

