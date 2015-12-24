import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import scylla.ssl

import quotaserver
from twisted.application import service, internet
from twisted.web import server

r = quotaserver.Server()
ctx_factory = scylla.ssl.VerifyingSSLContextFactory('zfsquota')
application = service.Application("HCS ZFS Quota Server")
internet.SSLServer(8081, server.Site(r),
    ctx_factory).setServiceParent(application)
