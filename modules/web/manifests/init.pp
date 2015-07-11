# web configuration
# will install and configure apache2 and other software required to run web servers
class web {

  require nfs
  $domain = hiera('domain')

  # PACKAGES
    
  # Apache  
  package { 'apache2':
    ensure => installed,
  } ->
  service { 'apache2':
    ensure => running,
    enable => true,
  }

  # Apache modules
  package { 'apache2-suexec-custom': }
  
  package { 'libapache2-mod-php5': }
  # Custom PHP configs. Changes include:
  # - Higher file upload size
  # - Use NFS session directory
  # - Enable php5 mcrypt mod
  file {'/etc/php5/cgi/php.ini':
    ensure  => file,
    source  => 'puppet:///modules/web/php5/cgi/php.ini',
    owner   => root,
    group   => root,
    notify  => Service['apache2'],
    require => Package['libapache2-mod-php5'],
  }
  file {'/etc/php5/apache2/php.ini':
    ensure  => file,
    source  => 'puppet:///modules/web/php5/apache2/php.ini',
    owner   => root,
    group   => root,
    notify  => Service['apache2'],
    require => Package['libapache2-mod-php5'],
  }
  
  package { 'libapache2-mod-suphp': }
  # Custom suphp conf which disables checking the Document root. If we don't
  # do this it errors because we symlink our /var/www to
  # /mnt/tank/hcs.harvard.edu
  file { '/etc/suphp/suphp.conf':
    ensure  => file,
    source  => 'puppet:///modules/web/suphp/suphp.conf',
    owner   => root,
    group   => root,
    notify  => Service['apache2'],
    require => Package['libapache2-mod-suphp']
  }

  package { 'libapache2-mod-fcgid': }

  package { 'libapache2-mod-wsgi': }
  
  # Perl modules
  package { 'libapache-session-perl': }
  package { 'libapache2-mod-perl2': }

  # Python packages
  package { 'libapache2-mod-python': }

  # RT
  package { 'request-tracker4':
    require => Package['apache2']
  }
  package { 'rt4-apache2':
    require => Package['request-tracker4']
  }
  package { 'rt4-clients':
    require => Package['request-tracker4']
  }
  package { 'libnet-ldap-perl':
    require => Package['request-tracker4']
  }

  # Shibboleth packages
  package { 'libapache2-mod-shib2':
    ensure => installed
  } ->
  # HUIT IDP metadata
  file { '/etc/shibboleth/huit-idp-metadata.xml':
    ensure => file,
    source => 'puppet:///modules/web/shibboleth/huit-idp-metadata.xml',
    owner  => 'root',
    group  => 'root'
  } ->
  # Main shibboleth configuration file
  file { '/etc/shibboleth/shibboleth2.xml':
    ensure  => file,
    source => 'puppet:///modules/web/shibboleth/shibboleth2.xml',
    owner   => 'root',
    group   => 'root',
    notify  => [Service['shibd'], Service['apache2']],
  } ->
  service { 'shibd':
    ensure  => running,
    enable  => true,
    require => Package['apache2']
  }

  # PHP5 modules
  web::php5_mod { 'mcrypt': }
  web::php5_mod { 'gd': }
  web::php5_mod { 'mysql': }

  # APACHE CONFIGURATION

  # This is the main apache configuration file. It sets high level directives
  # and includes sites-enabled and conf-enabled
  web::apache2_config { 'apache2.conf': }
    
  # Security configurations to limit exposing ServerTokens and ServerSignature.
  web::apache2_config { 'conf-available/security.conf': }

  # create the hcs conf directories
  file{ [ '/etc/apache2/hcs-conf',
          '/etc/apache2/hcs-nonsecure-conf',
          '/etc/apache2/hcs-ssl-conf' ]:
    ensure  => directory,
    require => Package['apache2'],
  }

  # These do spiffy HCS specific things like redirects for special people,
  # hosting from user directories and removing the tilde. These are applied to
  # secure and non-secure pages.
  web::apache2_config { 'hcs-conf/directories.conf': }
  web::apache2_config { 'hcs-conf/redirects.conf': }
  web::apache2_config { 'hcs-conf/shibboleth.conf': }
  web::apache2_config { 'hcs-conf/tilde-rewrites.conf': }

  # HCS configurations for non-secure pages
  web::apache2_config { 'hcs-nonsecure-conf/redirects.conf': }

  # HCS configurations for secure pages
  web::apache2_config { 'hcs-ssl-conf/helios.conf': }
  web::apache2_config { 'hcs-ssl-conf/phpmyadmin.conf': }
  web::apache2_config { 'hcs-ssl-conf/rt.conf': }
  
  # Remove default vhost
  file { '/etc/apache2/sites-enabled/000-default.conf':
    ensure  => absent,
    require => Package['apache2'],
  }

  # HCS enabled virtual hosts.
  #web::apache2_vhost{ 'default': }
  web::apache2_vhost{ 'hcs.harvard.edu': }
  web::apache2_vhost{ 'hcs.harvard.edu-ssl': }
  #web::apache2_vhost{ 'mail.hcs.harvard.edu': }
  #web::apache2_vhost{ 'secure.hcs.harvard.edu': }
  web::apache2_vhost{ 'user-vhosts': }

  # Mods enabled and disabled
  web::apache2_mod { 'actions': }
  web::apache2_mod { 'authnz_ldap': }
  web::apache2_mod { 'cgi': }
  web::apache2_mod { 'dav': }
  web::apache2_mod { 'dav_fs': }
  web::apache2_mod { 'fcgid':
    require => Package['libapache2-mod-fcgid'],
  }
  web::apache2_mod { 'headers': }
  web::apache2_mod { 'include': }
  web::apache2_mod { 'ldap': }
  web::apache2_mod { 'php5':
    ensure  => disabled,
    require => Package['libapache2-mod-php5'],
  }
  web::apache2_mod { 'python':
    require => Package['libapache2-mod-python'],
  }
  web::apache2_mod { 'rewrite': }
  web::apache2_mod { 'shib2':
    require => Package['libapache2-mod-shib2'],
  }
  web::apache2_mod { 'ssl': }
  web::apache2_mod { 'suexec': }
  web::apache2_mod { 'suphp':
    require => Package['libapache2-mod-suphp'],
  }
  web::apache2_config { 'mods-available/userdir.conf': }
  web::apache2_mod { 'userdir': }
  web::apache2_mod { 'wsgi':
    require => Package['libapache2-mod-wsgi'],
  }

  # Certificates
  file {'/etc/ssl/certs/hcs_harvard_edu_cert.cer':
    ensure => file,
    source => 'puppet:///modules/web/certs/hcs_harvard_edu_cert.cer',
    owner  => root,
    group  => root,
    mode   => '0644'
  }

  file {'/etc/ssl/certs/hcs_harvard_edu_interm.cer':
    ensure => file,
    source => 'puppet:///modules/web/certs/hcs_harvard_edu_interm.cer',
    owner  => root,
    group  => root,
    mode   => '0644'
  }

  # HCS super private key
  file {'/etc/ssl/private/star.hcs.harvard.edu.key':
    ensure  => file,
    content => hiera('star.hcs.harvard.edu.key'),
    owner   => root,
    group   => root,
    mode    => '0400',
  }

  # Symlink our web files to appropriate location
  file { '/var/www/hcs.harvard.edu':
    ensure  => link,
    target  => '/mnt/tank/www-hcs.harvard.edu',
    force   => true,
    owner   => 'root',
    group   => 'root',
    # Must have mounted www-hcs.harvard.edu
    require => [Nfs::Client::Mount['www-hcs.harvard.edu'], Package['apache2']],
  }

  file { '/var/www/hcs.harvard.edu-ssl':
    ensure  => link,
    target  => '/mnt/tank/www-hcs.harvard.edu-ssl',
    force   => true,
    owner   => 'root',
    group   => 'root',
    # Must have mounted www-hcs.harvard.edu-ssl
    require => [Nfs::Client::Mount['www-hcs.harvard.edu-ssl'], Package['apache2']],
  }
  
}
