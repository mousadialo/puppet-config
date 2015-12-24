import settings, xmlrpclib
from twisted.internet import defer
from twisted.web import client
from urlparse import urljoin
from urllib import urlencode
import logging
from twisted.python.failure import Failure
logger = logging.getLogger('cas')
import hashlib
from datetime import datetime, timedelta

# Comment out proxyticket stuff due to elixir incompatibility.
# It wasn't in use anyway.
#from cas_store.models import ProxyTicket
#import elixir
#elixir.setup_all()

try:
  from xml.etree import ElementTree
except ImportError:
  from elementtree import ElementTree

class AuthFailure(Exception):
  pass

class ExpiredTicketFailure(Exception):
  pass

def verifyCallback(response):
  print response
  tree = ElementTree.fromstring(response)
  if tree[0].tag.endswith('authenticationSuccess'):
    user = tree[0][0].text
    logger.info('Used CAS to authenticate %s' % user)
    return user
  else:
    logger.error('CAS authentication failed.')
    raise Failure('CAS authentication failed.',AuthFailure)

def check_db(result, ticket):
  logger.info('Checking DB for ticket')
  p = ProxyTicket.get(unicode(hashlib.md5(ticket).hexdigest()))
  if p is None:
    return None
  if p.is_expired():
    logger.info('Ticket expired')
      ##Todo: Raise an error here indicating that a new ticket is required##
    p.delete()
    elixir.session.flush()
    raise Failure("CAS proxy ticket expired",ExpiredTicketFailure)
  else:
    logger.info('Ticket found for %s' % p.hcs_username)
    return p.hcs_username


def pass_error(error):
  logger.info(error)
  raise error

def verify(ticket, service):
  logger.info(datetime.now())
  """Verifies CAS 2.0+ XML-based authentication ticket.

  Returns username on success and None on failure.
  """
  d = defer.succeed(None)
  d.addCallback(check_db, ticket)
  d.addErrback(pass_error)
  d.addCallback(_verify, ticket, service)
  return d

def writeToDB(result, ticket):
  ProxyTicket.table.delete(ProxyTicket.c.hcs_username==unicode(result)).execute()
  p = ProxyTicket(proxy_ticket=unicode(hashlib.md5(ticket).hexdigest()), hcs_username=unicode(result))
  elixir.session.flush()
  return result

def _verify(result, ticket,service):
  if result:
    return result
  params = {'ticket': ticket, 'service': service}
  url = (urljoin(settings.CAS_SERVER_URL, 'proxyValidate') + '?' +
         urlencode(params))
  d = client.getPage(url)
  d.addCallback(verifyCallback)
  d.addCallback(writeToDB, ticket)
  return d
