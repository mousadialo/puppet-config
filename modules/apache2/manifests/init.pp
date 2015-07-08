# apache2 configuration
# will install and configure apache2
# web servers will get more specific configuration
class apache2 {

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
    source  => 'puppet:///modules/apache2/php5/cgi/php.ini',
    owner   => root,
    group   => root,
    notify  => Service['apache2'],
    require => Package['libapache2-mod-php5']
  }
  file {'/etc/php5/apache2/php.ini':
    ensure  => file,
    source  => 'puppet:///modules/apache2/php5/apache2/php.ini',
    owner   => root,
    group   => root,
    notify  => Service['apache2'],
    require => Package['libapache2-mod-php5']
  }
  
  package { 'libapache2-mod-suphp': }
  # Custom suphp conf which disables checking the Document root. If we don't
  # do this it errors because we symlink our /var/www to
  # /mnt/tank/hcs.harvard.edu
  file { '/etc/suphp/suphp.conf':
    ensure  => file,
    source  => 'puppet:///modules/apache2/suphp/suphp.conf',
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
    source => 'puppet:///modules/apache2/shibboleth/huit-idp-metadata.xml',
    owner  => 'root',
    group  => 'root'
  } ->
  # Main shibboleth configuration file
  file { '/etc/shibboleth/shibboleth2.xml':
    ensure  => file,
    content => template('apache2/shibboleth/shibboleth2.xml.erb'),
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
  apache2::php5_mod { 'mcrypt': }
  apache2::php5_mod { 'gd': }
  apache2::php5_mod { 'mysql': }

  # APACHE CONFIGURATION

  # This is the main apache configuration file. It sets high level directives
  # and includes sites-enabled and conf-enabled
  apache2::config_file { 'apache2.conf': }
    
  # Security configurations to limit exposing ServerTokens and ServerSignature.
  apache2::config_file { 'conf-available/security.conf': }

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
  apache2::config_file { 'hcs-conf/directories.conf': }
  apache2::config_file { 'hcs-conf/redirects.conf': }
  apache2::config_file { 'hcs-conf/tilde-rewrites.conf': }

  # HCS configurations for non-secure pages
  apache2::config_file { 'hcs-nonsecure-conf/redirects.conf': }

  # HCS configurations for secure pages
  apache2::config_file { 'hcs-ssl-conf/helios.conf': }
  apache2::config_file { 'hcs-ssl-conf/phpmyadmin.conf': }
  apache2::config_file { 'hcs-ssl-conf/rt.conf': }
  
  # Remove default vhost
  file { '/etc/apache2/sites-enabled/000-default.conf':
    ensure => absent
  }

  # HCS enabled virtual hosts.
  #apache2::vhost{ 'default': }
  apache2::vhost{ 'hcs.harvard.edu': }
  apache2::vhost{ 'hcs.harvard.edu-ssl': }
  #apache2::vhost{ 'mail.hcs.harvard.edu': }
  #apache2::vhost{ 'secure.hcs.harvard.edu': }
  apache2::vhost{ 'user-vhosts': }

  # Mods enabled and disabled
  apache2::mod { 'actions': }
  apache2::mod { 'authnz_ldap': }
  apache2::mod { 'cgi': }
  apache2::mod { 'dav': }
  apache2::mod { 'dav_fs':
    with_conf => true,
  }
  apache2::mod { 'fcgid':
    with_conf => true,
  }
  apache2::mod { 'headers': }
  apache2::mod { 'include': }
  apache2::mod { 'ldap': }
  apache2::mod { 'php5':
    ensure    => disabled,
    with_conf => true,
  }
  apache2::mod { 'python': }
  apache2::mod { 'proxy':
    with_conf => true,
  }
  apache2::mod { 'proxy_ajp': }
  apache2::mod { 'proxy_balancer': }
  apache2::mod { 'proxy_http': }
  apache2::mod { 'rewrite': }
  apache2::mod { 'shib2': }
  apache2::mod { 'slotmem_shm': }
  apache2::mod { 'socache_shmcb': }
  apache2::mod { 'ssl':
    with_conf => true,
  }
  apache2::mod { 'suexec': }
  apache2::mod { 'suphp':
    with_conf => true,
  }
  apache2::config_file { 'mods-available/userdir.conf': }
  apache2::mod { 'userdir':
    with_conf => true,
  }
  apache2::mod { 'wsgi':
    with_conf => true,
  }

  # Certificates
  file {'/etc/ssl/certs/hcs_harvard_edu_cert.cer':
    ensure => file,
    source => 'puppet:///modules/apache2/certs/hcs_harvard_edu_cert.cer',
    owner  => root,
    group  => root,
    mode   => '0644'
  }

  file {'/etc/ssl/certs/hcs_harvard_edu_interm.cer':
    ensure => file,
    source => 'puppet:///modules/apache2/certs/hcs_harvard_edu_interm.cer',
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
    require => Nfs::Client::Mount['www-hcs.harvard.edu'],
  }

  file { '/var/www/hcs.harvard.edu-ssl':
    ensure  => link,
    target  => '/mnt/tank/www-hcs.harvard.edu-ssl',
    force   => true,
    owner   => 'root',
    group   => 'root',
    # Must have mounted www-hcs.harvard.edu-ssl
    require => Nfs::Client::Mount['www-hcs.harvard.edu-ssl'],
  }
  
}
