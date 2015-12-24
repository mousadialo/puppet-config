"""
Robust cert + proxy ticket backend
"""

from twisted.web import xmlrpc, server
from twisted.internet import defer
import xmlrpclib
import httplib
import utils
import cas
from scylla import ssl
from scylla import utils
import logging, settings
logger = logging.getLogger('scylla')

class UnifiedXMLRPCServer(xmlrpc.XMLRPC):
  url = None
  def __init__(self, url):
    self.url = url

  def callFunction(self, authuser, calluser, function, args):
    # Pass in calling user and authuser as first arg
    if not authuser:
      authuser = calluser
    args = [calluser, authuser] + args
    return defer.maybeDeferred(function, *args)

  def render(self, request):
    """Override 'render' method to support cas ticket -> client cert
       translation."""

    calluser, authuser = (None, None)
    if request.isSecure():
      calluser = request.channel.transport.getPeerCertificate(
        ).get_subject().commonName

    request.content.seek(0, 0)
    args, functionPath = xmlrpclib.loads(request.content.read())
    logger.info("Calluser %s called function %s" % (calluser, functionPath))
    try:
      function = self._getFunction(functionPath)
    except xmlrpclib.Fault, f:
      self._cbRender(f, request)
    else:
      if len(args) == 0:
        f = xmlrpclib.Fault(self.FAILURE, 'At least one argument expected')
        return self._cbRender(f, request)

      # First arg is cas ticket, or ''
      args = list(args)
      ticket = args[0]
      args = args[1:]
      logger.debug("DEV MODE")
      request.setHeader("content-type", "text/xml")
      # Only the acctpanel user is allowed to proxy for someone else.
      # DEBUG HACK!  Change to acctpanel
      if calluser == 'acctpanel' and ticket:
        d = cas.verify(ticket, "scylla") # Get the authuser
      else:
        d = defer.succeed(None)
      def handleError(x):
        logger.error(x)
        if x.type == cas.AuthFailure:
          return self._cbRender(xmlrpclib.Fault(604,"CAS authentication failed"), request)
        elif x.type == cas.ExpiredTicketFailure:
          return self._cbRender(xmlrpclib.Fault(605,"CAS ticket expired"), request)
        if settings.MODE == 'dev':
          # DEV mode only.  Should not really pass to user
          f = xmlrpclib.Fault(self.FAILURE, str(x.value))
        else:
         f = xmlrpclib.Fault(self.FAILURE, 'Something went wrong')
        return self._cbRender(f, request)
      d.addErrback(handleError)
      d.addCallback(self.callFunction, calluser, function, args)
      d.addCallback(self._cbRender, request)
    return server.NOT_DONE_YET

  @property
  def allowNone(self):
    return True


class UnifiedTransport(xmlrpclib.Transport):
  def __init__(self, service_user, *args):
    """
    Can pass in service_user, or None if this is just an actual user.
    xmlrpclib.Transport is a classic class, so can't use super.
    """
    self.service_user = service_user
    self.is_service_user = service_user is not None
    # super(UnifiedTransport, self).__init__(*args)
    xmlrpclib.Transport.__init__(self, *args)

  def make_connection(self, host):
    host, extra_headers, x509 = self.get_host_info(host)
    return httplib.HTTPS(host, None,
        key_file=utils.GetKeyPath(self.service_user, self.is_service_user),
        cert_file=utils.GetCertificatePath(self.service_user, self.is_service_user))

class UnifiedClientProxy(object):
  """
  Use for cert or CAS- auth client.  Stolen from Twisted library.
  Only difference is that __request can change the params given to it.
  """
  
  def __init__(self, uri, transport=None, encoding=None, verbose=0,
               allow_none=0, use_datetime=0, service_user=None):
    # establish a "logical" server connection
    
    # get the url
    import urllib
    type, uri = urllib.splittype(uri)
    if type not in ("http", "https"):
      raise IOError, "unsupported XML-RPC protocol"
    self.__host, self.__handler = urllib.splithost(uri)
    if not self.__handler:
      self.__handler = "/RPC2"

    if transport is None:
      transport = UnifiedTransport(service_user)
    self.__transport = transport

    self.__encoding = encoding
    self.__verbose = verbose
    self.__allow_none = allow_none
    self.proxy_ticket = None

  def __request(self, methodname, params):
    # call a method on the remote server
    # No proxy ticket for you!
    newparams = self.make_new_params(methodname, params)
    request = xmlrpclib.dumps(newparams, methodname, encoding=self.__encoding,
                              allow_none=self.__allow_none)
    
    response = self.__transport.request(
      self.__host,
      self.__handler,
      request,
      verbose=self.__verbose
      )

    if len(response) == 1:
      response = response[0]

    return response

  def __repr__(self):
    return (
      "<ServerProxy for %s%s>" %
      (self.__host, self.__handler)
      )
  
  __str__ = __repr__

  def __getattr__(self, name):
    # magic method dispatcher
    return xmlrpclib._Method(self.__request, name)

  # note: to call a remote object with an non-standard name, use
  # result getattr(server, "strange-python-name")(args)


class CertClientProxy(UnifiedClientProxy):
  """
  Use for cert-based auth client.
  """
  def make_new_params(self, methodname, params):
    """
    newparams should have an empty string as the proxy ticket.
    """
    return ('',) + tuple(params)


class ClientProxy(UnifiedClientProxy):
  """
  Use for auth where the client passes a proxy ticket (or empty str)
  """
  def make_new_params(self, methodname, params):
    """
    Add the proxy ticket
    """
    return (self.proxy_ticket, ) + tuple(params)
