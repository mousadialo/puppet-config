#!/usr/bin/env ruby
# This script was borrowed from carl@hcs (github: @zenzen) 

require 'yaml'

# I would have done this with Hiera's `heira_include`, but this is more
# flexible, and plus it allows us to pass interesting things to Hiera later.

node = ARGV[0]
raise "No host provided" if node.nil?

hosts = YAML.load_file(File.join(File.dirname(__FILE__), 'data', 'hosts.yaml'))

# For our purposes, use regex to pull out the server type from the hostname
node = node[/^[a-z][a-z]*/,0]
raise "Unable to determine server type" if node.nil?

print YAML.dump(hosts.fetch(node))
