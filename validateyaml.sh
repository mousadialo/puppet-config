#!/bin/bash

echo "Validating YAML files. Should not print an error if valid."
# find all yaml files
FILES=`find ./ -type f -name \*.*yaml`
# try to load the yaml file with ruby. if it fails, then we have bad yaml!
for YAML in $FILES; do
  echo "Validating ${YAML} ..."
  ruby -e "require 'yaml'; YAML.load_file('${YAML}')"
done
