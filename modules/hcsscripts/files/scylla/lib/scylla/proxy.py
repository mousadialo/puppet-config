# CAS -> Scylla Proxy for allowing webapps to easily make
# requests to scylla backends without needing to use ssh.

# This could be implemented over ssh hijacking it as the 
# only priviledged process, but this implementation is
# simpler, and probably just as secure especially if
# coupled with an apparmor profile restricting the 
# access of this script (or judicious use of
# posix groups for scylla keys)

from twisted.web import xmlrpc
import xmlrpclib
from scylla import ssl
from scylla import utils

class CertProxy(xmlrpc.Proxy):
  def callRemote(self, method, *args):
    # I'm sorry that there design is poor and doesn't support
    # certs.  I don't want to use their private methods, but there
    # isn't really a choice
    factory = xmlrpc._QueryFactory(
        self.path, self.host, method, self.user,
        self.password, self.allowNone, args)
    if self.secure:
        from twisted.internet import ssl 
        reactor.connectSSL(self.host, self.port or 443,
                           factory, CertClientContextFactory(self.user))
    else:
        reactor.connectTCP(self.host, self.port or 80, factory)
    return factory.deferred

class ProxyServer(xmlrpc.XMLRPC):
  url = None
  def __init__(self, url):
    self.url = url

  def callFunction(self, functionPath, args, user):
    proxy = ssl.CertProxy(url, user=user)
    return proxy.callRemote(functionPath, *args)

  def render(self, request):
    """Override 'render' method to support cas ticket -> client cert
       translation."""
    user = None

    request.content.seek(0, 0)
    args, functionPath = xmlrpclib.loads(request.content.read())
    # First arg is cas ticket
    args = list(args)
    ticket = args[0]
    d = cas.verify(ticket, "scylla")
    d.addCallback(callFunction, functionPath, args[1:])
    d.addCallback(self._cbRender, request)
    d.addErrback(self._ebRender)

    return server.NOT_DONE_YET
