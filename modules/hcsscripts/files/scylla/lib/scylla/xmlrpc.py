from twisted.web import xmlrpc, server
from twisted.internet import defer
import xmlrpclib
import httplib

import utils

class XmlRpcTransport(xmlrpclib.Transport):
  def make_connection(self, host):
    host, extra_headers, x509 = self.get_host_info(host)
    return httplib.HTTPSConnection(host, None,
        key_file=utils.GetKeyPath(),
        cert_file=utils.GetCertificatePath())

class ClientCertXMLRPC(xmlrpc.XMLRPC):
  def render(self, request):
    """Override 'render' method to support client certs"""
    user = None
    if request.isSecure():
      user = request.channel.transport.getPeerCertificate(
          ).get_subject().commonName

    request.content.seek(0, 0)
    args, procedurePath = xmlrpclib.loads(request.content.read())
    args = list(args)
    args.insert(0, user)
    try:
      function = self.lookupProcedure(procedurePath)
    except xmlrpclib.Fault, f:
      self._cbRender(f, request)
    else:
      request.setHeader("content-type", "text/xml")
      defer.maybeDeferred(function, *args).addErrback(
          self._ebRender
          ).addCallback(
          self._cbRender, request
          )

    return server.NOT_DONE_YET

