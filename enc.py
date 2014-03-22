import yaml
import sys
import re

# basic error checking
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
    print "Couldn't extract a server type from hostname!"
    sys.exit(1)

try:
    hosts = open("data/hosts.yaml")
    data = yaml.load(hosts)
    print "---\n" + yaml.dump(data[template_type], default_flow_style=False).rstrip()
except:
    print "Something went wrong! Are you sure the yaml config exists?"
