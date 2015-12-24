# Scylla is a simple mechanism for authenticating HCS account holders to
# HCS backend services by means of X509 certificates.  It is essentially
# a wrapper around SSL that creates SSL contexts suitable for connecting
# to HCS service servers and for such servers to verify client authenticity.
# 
# Author: gdasher@hcs.harvard.edu (Grant Dasher)
# Some of this code is due to Eli Criffield and is obtained from
# http://twistedmatrix.com/pipermail/twisted-python/2007-May/015357.html

import pdb
from twisted.internet import ssl
from OpenSSL import crypto
from OpenSSL import SSL
import utils

class CertClientContextFactory(ssl.ClientContextFactory):
  user = None
  def __init__(self, user):
    self.user = user

  def getContext(self):
    c = SSL.Context(self.method)
    c.use_certificate_file(utils.GetCertificatePath(user=self.user))
    c.use_privatekey_file(utils.GetKeyPath(user=self.user))
    return c

class VerifyingSSLContextFactory(ssl.DefaultOpenSSLContextFactory):
  def __init__(self, service):
    ssl.DefaultOpenSSLContextFactory.__init__(self, 
      utils.GetServerKeyPath(service),
      utils.GetServerCertificatePath(service))

  def verify_cb(self, conn, cert, errnum, depth, ok):
    return ok == 1

  def cacheContext(self):
    ctx = SSL.Context(self.sslmethod)
    ctx.use_certificate_file(self.certificateFileName)
    ctx.use_privatekey_file(self.privateKeyFileName)
    ctx.load_client_ca(utils.CA_CERT)
    ctx.load_verify_locations(utils.CA_CERT)
    ctx.set_verify(SSL.VERIFY_PEER |
        SSL.VERIFY_FAIL_IF_NO_PEER_CERT, self.verify_cb)
    ctx.set_verify_depth(1)
    self._context = ctx

