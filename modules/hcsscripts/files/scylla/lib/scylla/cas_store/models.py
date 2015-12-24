#import hcs.passwords
from datetime import datetime, timedelta
from elixir import *
from scylla import settings

# TODO: don't use the root MySQL password by adding a mysql-cas_cache password
# to /etc/hcs/passwords/ and ship it on web servers.
# In the meantime, these lines are commented out.
#password = hcs.passwords.get('mysql')
#metadata.bind = ('mysql://root:%s@mysql.hcs.harvard.edu/cas_cache' % password)

class ProxyTicket(Entity):
    using_options(tablename='cas_store_models_proxyticket')
    proxy_ticket = Field(Unicode(255),primary_key=True)
    hcs_username = Field(Unicode(255))
    date_created = Field(DateTime)
    
    def __init__(self, proxy_ticket, hcs_username):
        self.proxy_ticket = proxy_ticket
        self.hcs_username = hcs_username
        self.date_created = datetime.utcnow()

    def is_expired(self):
        #Set 30 min expiry time
        exp = ((datetime.utcnow()-self.date_created) > timedelta(minutes=30))
        return exp


