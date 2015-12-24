import pwd
import os
import os.path

KEYS_DIR = '.hcskeys'
CERT = 'cert.pem'
PRIV_KEY = 'key.pem'
SERVICE_KEY_DIR = '/etc/hcs/scylla_services'
CA_CERT = '/etc/hcs-scylla/ca.pem'

def _GetUser():
  return pwd.getpwuid(os.getuid())[0]

def GetUidAndGid(user=_GetUser()):
  pwinfo = pwd.getpwnam(user)
  return pwinfo[2], pwinfo[3]

def GetKeyDir(user=_GetUser()):
  return os.path.expanduser(os.path.join('~' + user, KEYS_DIR))

def GetCertificatePath(user=None, service_user=False):
  if not service_user:
    if not user:
      user = _GetUser()
    return os.path.join(GetKeyDir(user), CERT)
  else:
    return GetServerCertificatePath(user)

def GetKeyPath(user=None, service_user=False):
  if not service_user:
    if not user:
      user = _GetUser()
    return os.path.join(GetKeyDir(user), PRIV_KEY)
  else:
    return GetServerKeyPath(user)

def GetServerCertificatePath(service):
  return os.path.join(SERVICE_KEY_DIR, '%s_cert.pem' % service)

def GetLocalServerCertificatePath(service):
  """Get path in pwd for dumping cert to."""
  return os.path.join(os.path.abspath('.'), '%s_cert.pem' % service)

def GetServerKeyPath(service):
  return os.path.join(SERVICE_KEY_DIR, '%s_key.pem' % service)

def GetLocalServerKeyPath(service):
  """Get path in pwd for dumping key to."""
  return os.path.join(os.path.abspath('.'), '%s_key.pem' % service)

def VerifyAccountScyllaReady(user=_GetUser()):
  if not (os.access(GetCertificatePath(user), os.R_OK) and
      os.access(GetKeyPath(user), os.R_OK)):
    raise Exception("Your account is not configured correctly.  Please " +
        "contact acctserv@hcs and tell them to enable your account's " +
        "certificates.")
