#!/usr/bin/env python

import yaml
import os
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

machine_types = [os.path.splitext(m)[0] for m in os.listdir(os.path.join(os.path.dirname(os.path.realpath(__file__)), "data", "machines"))]

try:
    template_type = re.search('^[a-z][a-z]*', node).group(0)
    if template_type not in machine_types:
        template_type = "generic"
except AttributeError:
    template_type = "generic"

try:
    hosts = open(os.path.join(os.path.dirname(os.path.realpath(__file__)), "data", "hosts.yaml"))
    data = yaml.load(hosts)
    print yaml.dump(data[template_type], explicit_start=True, default_flow_style=False).rstrip()
except:
    print "Something went wrong! Are you sure the yaml config exists?"
