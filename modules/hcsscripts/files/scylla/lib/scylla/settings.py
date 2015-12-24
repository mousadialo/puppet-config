import re, os
CAS_SERVER_URL = 'https://secure.hcs.harvard.edu'
if re.match('^/(home|nfs)/', os.path.abspath(__file__)):
    MODE = 'dev'
else:
    MODE = 'prod'
