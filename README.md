HCS Puppet Configuration
=========================

### INSTALLATION
`git submodule init`
`git submodule update`

### WRITING PUPPET
When writing puppet configuration, you should be sure to lint your manifests to conform to the style guide. This will help catch minor bugs as well.

#### Linter
Install the puppet-lint gem. You'll need rubygem (and Ruby).
`apt-get install rubygems`
`gem install puppet-lint`

To run the puppet linter:
`puppet-lint --with-filename <directory>`

To fix warnings and errors, look them up on puppet-lint.com.




TODOs
- script to connect agent to master
- fix apache2 bugs
- config for mail and mailman
- configuration for timezone (base class) set to US EAST

MAIL
-----
- postfix is loading. need to figure out how to make it work properly...
- dovecot is loading, need to test it though!
- setup postgrey
- postfix: figure out VRFY TODO in master.cf (lists ip address)


APACHE
----
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

