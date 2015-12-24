import re, os, sys
path = os.path.join(os.getcwd(), __file__)
# If this file is in /usr, then run in production mode.
if re.search('^/usr/', path):
    mode = 'production'
else:
    mode = 'development'
    sys.path = ['/home/people/brockman/repositories/svn/trunk/packages/python-hcs'] + sys.path
