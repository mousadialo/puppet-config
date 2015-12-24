#!/usr/bin/env python
from xmlrpclib import ServerProxy, Error
import sys

"""
Check to see if the argument is a mailing list or mail alias.  Used by
group-add.sh to prevent name collisions.  Returns 0 if it is a list,
1 otherwise
"""

def isnameinuse(listname):
  server = ServerProxy("http://trajan.internal.hcs.harvard.edu:8080/")
  return server.isnameinuse(listname)

def main(argv):
  if len(argv) != 2:
    print "usage: %s <name>" % argv[0]
    return -1

  try:
    if isnameinuse(argv[1]):
      print "%s is a list or mail alias " % argv[1]
      return 0
    else:
      print "%s is not a list or mail alias" % argv[1]
      return -1
  except Error, e:
    print "ERROR", e

if __name__ == "__main__":
  sys.exit(main(sys.argv))

