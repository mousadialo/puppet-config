import yaml
import sys
import re

if len(sys.argv) < 2:
    print "No host provided!"
    sys.exit(0)

node = sys.argv[1]

if node is None:
    print "No host provided!"
    sys.exit(0)

try:
    template_type = re.search('^[a-z][a-z]*', node).group(0)
except AttributeError:
    print "Didn't find a server type from hostname!"
    sys.exit(1)

try:
    template = open("data/"+template_type+".yaml", 'r')
    print yaml.load(template)
except:
    print "Something went wrong! Are you sure the yaml config exists?"
