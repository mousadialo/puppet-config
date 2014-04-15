HCS Puppet Configuration
=========================

### INSTALLATION
Just pull this repository.
You'll want to place it in `/etc/puppet` on a puppetmaster server.

### WRITING PUPPET
When writing puppet configuration, you should be sure to lint your manifests to conform to the style guide. This will help catch minor bugs as well.

#### VALIDATE
Install puppet-common on your machine. You may need the puppetlabs repository.
http://docs.puppetlabs.com/guides/puppetlabs_package_repositories.html

`sudo apt-get puppet-common`

Validate a file by running the following command.
`puppet puppet validate <FILENAME>`

(You can also validate the files by running puppet, but you should do this on a development server)



#### Linter
Install the puppet-lint gem. You'll need rubygem (and Ruby).
`apt-get install rubygems`
`gem install puppet-lint`

To run the puppet linter:
`puppet-lint --with-filename <directory>`

To fix warnings and errors, look them up on puppet-lint.com.




###TODOs
- script to connect agent to master
- fix apache2 bugs
- config mailman on the mail server
- test connection for bifrosts using the gateway config

###MAIL
- postfix is loading. need to figure out how to make it work properly...
- dovecot is loading, need to test it though!
- setup postgrey
- postfix: figure out VRFY TODO in master.cf (lists ip address)


###APACHE
Need to move over certs and keys specified in sites-availible/hcs.harvard.edu-ssl
mail.hcs.harvard.edu - add config and then setup roundcube
Find out how aquila proxying works
Do we need everything in conf.d/?
Redirects go to things like lists.harvard.edu. There is no vhosts for that?
RT authentication uses ldap, how do we get around this?
Log rotation in apache

phpmyadmin
rt
webmail with roundcube

###ZFS/NFS



