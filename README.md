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

Validate a file by running the following command: `puppet parser validate <FILENAME>`

(You can also validate the files by running puppet from a client, but you should do this on a development server)

#### Linter
Install the puppet-lint gem. You'll need rubygem (and Ruby).
-`apt-get install rubygems`
-`gem install puppet-lint`

To run the puppet linter: `puppet-lint --with-filename <directory>`
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


###LDAP Server Setup

#### Tools
LDAP Directory Server [this is required!!!] - this stores the LDAP ldifs (information) and can be queried by the clients for auth  
`sudo apt-get install 389-ds-base`  
LDAP Admin Server (sys admin server) - this is a tool that helps moderate the server and should be uninstalled after use.  
`sudo apt-get install 389-admin`  
LDAP Admin GUI - This is the GUI that connects to the admin for easy use.  
`sudo apt-get install 389-ds-console`  

#### Export data from existing server
You need to take a cut of the LDAP database into an ldif file.  
If using the Admin GUI  
- create a file (ex: domain.ldif) and run `chown dirsrv:dirsrv domain.ldif`
- you will need to ssh using -Y to forward X11 sessions to your computer
- then run `389-console &` to run the process in the background
- the GUI will pop up (it's pretty slow) but you can then export the database into `domain.ldif`

If using ldapsearch  
`ldapsearch -Wx -D "cn=Directory Manager" -b "dc=hcs,dc=harvard,dc=edu" > domain.ldif`  
The -W flag will ask for the LDAP password. And now we have a cut of the data.  

#### Importing data into new server
- Install the tools as per commands above.
- `sudo setup-admin`
- you can pretty much use all the defaults unless you know what you are doing.
- If RFC 4519 is still in use, copy the 00core.ldif file into /etc/dirsrv/slapd-<name>/ (you need to be root, use sudo -i)
- IF RFC 4519 is outdated, then you will probably have to modify the uniqueMember to use the LDAP Directory String type.
- Copy the ldap.conf file from `/puppet-config/modules/ldap-server/files/ldap/ldap.conf` (this repo) to the server: `/etc/ldap/ldap.conf`
- restart the dirsrv (`sudo service dirsrv restart`). any errors are logged in /var/log/dirsrv (again be root, sudo -i)
- run the 389-console and import the data
- you can now check the data by using `ldapsearch -x` (this won't work correctly if you didn't set up the ldap.conf)
