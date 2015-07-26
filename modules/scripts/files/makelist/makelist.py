from __future__ import with_statement

from Crypto.Cipher import AES
from twisted.web import xmlrpc
from email import Utils

import base64
import hmac
import os
import os.path
import pwd
import re
import smtplib
import subprocess
import time
import urllib
import xmlrpclib

def mail(to, message):
  mailServer = smtplib.SMTP('hcs.harvard.edu', '10025')
  mailServer.sendmail('systems@hcs.harvard.edu', to, message)
  mailServer.quit()

class Server(xmlrpc.XMLRPC):
  confirm_message = """\
From: HCS Account Services <acctserv@hcs.harvard.edu>
To: %(confirmation)s
Subject: Mailing list %(listname)s requires confirmation
Date: %(date)s
Message-ID: %(messageid)s

Somebody wants to create a mailing list at hcs.harvard.edu with your email as the
primary email contact.
List name: %(listname)s
Password: %(password)s
List admin: %(listadmin)s
Confirmation: %(confirmation)s

To create this list, please confirm that you submitted the above information by 
following this link:
%(confirmurl)s
"""

  confirm_password_message = """\
From: HCS Account Services <acctserv@hcs.harvard.edu>
To: %(recipients)s
Subject: Mailing list %(listname)s password reset requires confirmation
Date: %(date)s
Message-ID: %(messageid)s

Somebody wants to reset the password for a mailing list at hcs.harvard.edu for
which you are an administrator.
List name: %(listname)s

To reset the password, please confirm that you requested the change by following
this link:
%(confirmurl)s
"""

  log_success_message = """\
From: HCS Systems <systems@hcs.harvard.edu>
To: acctserv@hcs.harvard.edu
Subject: Mailing list %(listname)s successfully created
Date: %(date)s
Message-ID: %(messageid)s
X-No-Create-Ticket: true

List name: %(listname)s
Password: %(password)s
List admin: %(listadmin)s
Confirmation: %(confirmation)s

Here is mailman's opinion:
%(output)s
"""

  log_password_success_message = """\
From: HCS Systems <systems@hcs.harvard.edu>
To: acctserv@hcs.harvard.edu
Subject: Mailing list %(listname)s password reset
Date: %(date)s
Message-ID: %(messageid)s
X-No-Create-Ticket: true

List name: %(listname)s
List admin: %(listadmin)s
"""

  unknown_mailman_error = """\
From: HCS Systems <systems@hcs.harvard.edu>
To: systems@hcs.harvard.edu
Subject: Automatic mailing list creation failed for unknown reason
Date: %(date)s
Message-ID: %(messageid)s

Unknown error while trying to create list with trajan.hcs.harvard.edu/makelist:
List name: %(listname)s
Password: %(password)s
List admin: %(listadmin)s
Confirmation: %(confirmation)s

Here is mailman's output:
stdout:
%(output)s

stderr:
%(outerr)s
"""

  unknown_mailman_password_error = """\
From: HCS Systems <systems@hcs.harvard.edu>
To: systems@hcs.harvard.edu
Subject: Password reset failed for unknown reason
Date: %(date)s
Message-ID: %(messageid)s

Unknown error while trying to reset password for:
List name: %(listname)s
List admin: %(listadmin)s

Here is mailman's output:
stdout:
%(output)s

stderr:
%(outerr)s
"""



  def __init__(self):
    xmlrpc.XMLRPC.__init__(self)
    # A nice random 256-bit key
    keyfile = open('/usr/lib/makelist/keyfile', 'rb')
    self.secret_cipher = keyfile.read(32)
    self.secret_hmac = keyfile.read(32)
    keyfile.close()
    if len(self.secret_cipher) != 32 or len(self.secret_hmac) != 32:
      raise ValueError('keyfile was not at least 512 bits long')

  def Fault(self, msg):
    return xmlrpclib.Fault(self.FAILURE, msg)

  def computeMAC(self, message):
    return hmac.new(self.secret_hmac, message).digest()

  def xmlrpc_confirm(self, token, mac):
    def badconfirmlinkmsg():
      return self.Fault('Your confirm link was invalid; contact acctserv@hcs '\
          'further assistance.')

    token = (token or '').strip()
    mac = (mac or '').strip()
    cryptor = AES.new(self.secret_cipher, AES.MODE_ECB)

    try:
      plaintext = cryptor.decrypt(base64.urlsafe_b64decode(token))
      mac = base64.urlsafe_b64decode(mac)
    except ValueError: # if the code is a not a multiple of 16 bytes long
      raise badconfirmlinkmsg()
    except TypeError: # invalid padding
      raise badconfirmlinkmsg()

    if self.computeMAC(plaintext) != mac:
      raise badconfirmlinkmsg()

    # A proper listvars is of the form
    # [ listname, password, listadmin, confirmation, padding ]
    listvars = plaintext.split("\x00")
    if len(listvars) == 5:
      listname = listvars[0]
      password = listvars[1]
      listadmin = listvars[2]
      confirmation = listvars[3]

      # make list; else return error
      # os.system('/usr/lib/mailman/bin/newlist %s %s %s' % (listname, listadmin, password))
      p = subprocess.Popen(('/usr/lib/mailman/bin/newlist',
                        listname, listadmin, password),
                       stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      p.stdin.write('\n\n')
      p.stdin.close()
      retcode = p.wait()

      if retcode is 0:
        # return success message
        output = p.stdout.read()
        mail('acctserv@hcs.harvard.edu', self.log_success_message % {
            'date' : Utils.formatdate(localtime = True),
            'messageid' : Utils.make_msgid(),
            'listname' : listname,
            'password' : '...',
            'listadmin' : listadmin,
            'confirmation' : confirmation,
            'output' : output })

        return listname

      elif retcode is 16:
        raise self.Fault(
            'Sorry - list already exists. Choose a different list name.')
      else:
        # non-standard problem -- e-mail systems/acctserv
        output = p.stdout.read()
        outerr = p.stderr.read()
        mail('systems@hcs.harvard.edu', self.unknown_mailman_error % {
            'date' : Utils.formatdate(localtime = True),
            'messageid' : Utils.make_msgid(),
            'listname' : listname,
            'password' : password,
            'listadmin' : listadmin,
            'confirmation' : confirmation,
            'output' : output,
            'outerr' : outerr })
        # return unknown error
        raise self.Fault('Internal error. The systems team has been notified '\
            'and will be getting back to you.')

    else: # User submitted bad hash
      raise badconfirmlinkmsg()


  def xmlrpc_newlist(self, listname, password, listadmin, confirmation):
    def page(message):
      return self.page(message, listname, password, listadmin, confirmation)

    listname = (listname or '').strip()
    password = (password or '').strip()
    listadmin = (listadmin or '').strip()
    confirmation = (confirmation or '').strip()

    cryptor = AES.new(self.secret_cipher, AES.MODE_ECB)

    # check values are good; else return error
    if not listname or not password or not listadmin:
      raise self.Fault('Please fill in all fields.')

    if not re.search('^[0-9a-zA-Z\-_]+$',listname):
      raise self.Fault('Listname contains funny characters. Try again.')

    if not re.search('^[\+\.\w\-_]+@([\w\-_]+\.)+[a-zA-Z]{2,4}$', listadmin):
      raise self.Fault('List admin e-mail must be a real e-mail address.')

    if not confirmation:
      confirmation = listadmin
    if not re.search(r'@(.*\.)?harvard\.edu$', confirmation):
      raise self.Fault('Confirmation must be to a harvard address')

    hcsemail = re.match(r'([\+\.\w\-_]+)@hcs.harvard.edu', confirmation)
    if hcsemail != None and self.islist(hcsemail.group(1)):
      raise self.Fault('Cannot use a mailing list as a confirmation email.')

    # check if it exists as a list; if so, return error
    if self.islist(listname):
      raise self.Fault('Sorry - list already exists. Choose a different list name.')

    # check if it exists as an alias or user account
    if self.is_user_or_alias(listname):
      raise self.Fault('Sorry, the name "%s" is taken by an HCS account.' %\
          listname)

    # AES has a block size of 16 bytes.  By convention, pad the message at the
    # end with 16 - (len(message) % 16) bytes of value 16 - (len(message) % 16).
    # When the decrypted bytes are interpreted at the end, these pad bytes will
    # show up in the last part of the split list, and will be discarded.  For
    # this reason, make sure the message ends with a null character by including
    # an empty string in the list to join.
    message = '\x00'.join([listname, password, listadmin, confirmation,
                           ''])
    pad_size = 16 - (len(message) % 16)
    message += ''.join([chr(pad_size) for x in xrange(0, pad_size)])

    token = cryptor.encrypt(message)
    confirmurl = "http://www.hcs.harvard.edu/make-list?%s" % \
      urllib.urlencode(
          { 't' : base64.urlsafe_b64encode(token),
            'm' : base64.urlsafe_b64encode(self.computeMAC(message)) })
    

    # Send confirmation e-mail.
    mail(confirmation, self.confirm_message % {
        'date' : Utils.formatdate(localtime = True),
        'messageid' : Utils.make_msgid(),
        'listname' : listname,
        'password' : password,
        'listadmin' : listadmin,
        'confirmation' : confirmation,
        'confirmurl' : confirmurl })
    
    return True

  def islist(self, listname):
    return os.access(os.path.join('/var/lib/mailman/lists', listname), os.F_OK)

  def get_admins(self, listname):
      cmd = '/var/lib/mailman/bin/list_admins'
      p = subprocess.Popen([cmd, listname], stdout=subprocess.PIPE)
      out, err = p.communicate()
      out = out.split("Owners: ")
      return out[1].strip().split(', ')

  def is_user_or_alias(self, listname):
    s = smtplib.SMTP('hcs.harvard.edu', '10025')
    s.putcmd("VRFY %s@hcs.harvard.edu" % listname)
    reply = s.getreply()
    s.quit()
    # We return true if we get a code in the 200s, not 400+ (error)
    # (for some reason, we get 252 for 'user exists', and 400+ else, even though
    #  the rfc says 252 means 'Cannot VRFY user but will accept')
    return reply[0] <= 400

  def xmlrpc_isnameinuse(self, name):
    return self.islist(name) or self.is_user_or_alias(name)

  def xmlrpc_resetpassword(self, listname):
    def page(message):
      return self.page(message, listname, password, listadmin, confirmation)

    listname = (listname or '').strip()

    cryptor = AES.new(self.secret_cipher, AES.MODE_ECB)

    # check values are good; else return error
    if not listname:
      raise self.Fault('Please fill in all fields.')

    if not re.search('^[0-9a-zA-Z\-_]+$',listname):
      raise self.Fault('Listname contains funny characters. Try again.')

    # check if it exists as a list; if so, return error
    if not self.islist(listname):
      raise self.Fault('Sorry - list does not exist.')

    # AES has a block size of 16 bytes.  By convention, pad the message at the
    # end with 16 - (len(message) % 16) bytes of value 16 - (len(message) % 16).
    # When the decrypted bytes are interpreted at the end, these pad bytes will
    # show up in the last part of the split list, and will be discarded.  For
    # this reason, make sure the message ends with a null character by including
    # an empty string in the list to join.
    message = '\x00'.join([listname, ''])
    pad_size = 16 - (len(message) % 16)
    message += ''.join([chr(pad_size) for x in xrange(0, pad_size)])

    token = cryptor.encrypt(message)
    confirmurl = "http://www.hcs.harvard.edu/reset-list-password?%s" % \
      urllib.urlencode(
          { 't' : base64.urlsafe_b64encode(token),
            'm' : base64.urlsafe_b64encode(self.computeMAC(message)) })
    
    admins = self.get_admins(listname)

    # Send confirmation e-mail.
    mail(admins, self.confirm_password_message % {
        'date' : Utils.formatdate(localtime = True),
        'messageid' : Utils.make_msgid(),
        'listname' : listname,
        'recipients': ', '.join(admins),
        'confirmurl' : confirmurl })
    
    return True

  def xmlrpc_resetpassword_confirm(self, token, mac):
    def badconfirmlinkmsg():
      return self.Fault('Your confirm link was invalid; contact acctserv@hcs '\
          'further assistance.')

    token = (token or '').strip()
    mac = (mac or '').strip()
    cryptor = AES.new(self.secret_cipher, AES.MODE_ECB)

    try:
      plaintext = cryptor.decrypt(base64.urlsafe_b64decode(token))
      mac = base64.urlsafe_b64decode(mac)
    except ValueError: # if the code is a not a multiple of 16 bytes long
      raise badconfirmlinkmsg()
    except TypeError: # invalid padding
      raise badconfirmlinkmsg()
      
    if self.computeMAC(plaintext) != mac:
      raise badconfirmlinkmsg()

    # A proper listvars is of the form
    # [ listname, padding ]
    listvars = plaintext.split("\x00")
    if len(listvars) == 2:
      listname = listvars[0]
    
      # reset password; else return error
      p = subprocess.Popen(('/usr/lib/mailman/bin/change_pw', '-l', listname),
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      retcode = p.wait()

      admins = ', '.join(self.get_admins(listname))
      if retcode is 0:
        # return success message
        output = p.stdout.read()
        mail('acctserv@hcs.harvard.edu', self.log_password_success_message % {
            'date' : Utils.formatdate(localtime = True),
            'messageid' : Utils.make_msgid(),
            'listname': listname,
            'listadmin': admins})

        return listname

      else:
        # non-standard problem -- e-mail systems/acctserv
        output = p.stdout.read()
        outerr = p.stderr.read()
        mail('systems@hcs.harvard.edu', self.unknown_mailman_password_error % {
            'date' : Utils.formatdate(localtime = True),
            'messageid' : Utils.make_msgid(),
            'listname' : listname,
            'listadmin' : admins,
            'output' : output,
            'outerr' : outerr })
        # return unknown error
        raise self.Fault('Internal error. The systems team has been notified '\
            'and will be getting back to you.')
          
    else: # User submitted bad hash
      raise badconfirmlinkmsg()

