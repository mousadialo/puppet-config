import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import makelist
from twisted.application import service, internet
from twisted.web import server

r = makelist.Server()
application = service.Application("HCS Make List Server")
internet.TCPServer(8080, server.Site(r)).setServiceParent(application)
