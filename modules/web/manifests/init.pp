# web configuration
# will install and configure apache2 and other software required to run web servers
class web {
  
  $domain = hiera('domain')
  $mount_dir = hiera('nfs-mount-dir')
    
  # Apache  
  package { 'apache2':
    ensure => installed,
  } ->
  service { 'apache2':
    ensure => running,
    enable => true,
  }

  # This is the main apache configuration file. It sets high level directives
  # and includes sites-enabled and conf-enabled
  web::apache2::config { 'apache2.conf': }
    
  # Security configurations to limit exposing ServerTokens and ServerSignature.
  web::apache2::config { 'conf-available/security.conf': }

  if $::machine_type == 'web' {
    require nfs
    require certs
    
    # PACKAGES
    
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
      require => Package['apache2'],
    }
    package { 'rt4-apache2':
      require => Package['request-tracker4'],
    }
    package { 'rt4-clients':
      require => Package['request-tracker4'],
    }
    package { 'libnet-ldap-perl':
      require => Package['request-tracker4'],
    }
    exec { '/usr/bin/cpan -i RT::Authen::ExternalAuth':
      environment => 'PERL_MM_USE_DEFAULT=1',
      creates     => '/usr/local/share/request-tracker4/plugins/RT-Authen-ExternalAuth',
      require     => [Package['build-essential'], Package['request-tracker4']],
    }

    # Shibboleth packages
    package { 'libapache2-mod-shib2':
      ensure => installed
    }
    
    # Main shibboleth configuration file
    file { '/etc/shibboleth/shibboleth2.xml':
      ensure  => file,
      source  => 'puppet:///modules/web/shibboleth/shibboleth2.xml',
      owner   => 'root',
      group   => 'root',
      require => Package['libapache2-mod-shib2'],
      notify  => Service['shibd'],
    }
    
    # HUIT IDP metadata
    file { '/etc/shibboleth/huit-idp-metadata.xml':
      ensure  => file,
      source  => 'puppet:///modules/web/shibboleth/huit-idp-metadata.xml',
      owner   => 'root',
      group   => 'root',
      require => Package['libapache2-mod-shib2'],
      notify  => Service['shibd'],
    }
    
    # Attribute Map for HUIT IDP
    file { '/etc/shibboleth/attribute-map.xml':
      ensure  => file,
      source  => 'puppet:///modules/web/shibboleth/attribute-map.xml',
      owner   => 'root',
      group   => 'root',
      require => Package['libapache2-mod-shib2'],
      notify  => Service['shibd'],
    }
    
    service { 'shibd':
      ensure  => running,
      enable  => true,
      require => Package['libapache2-mod-shib2'],
    }
    
    # Packages needed by helios
    package { 'python-ldap': }
    package { 'python-flask': }

    # PHP5 modules
    web::php5::mod { 'mcrypt': }
    web::php5::mod { 'gd': }
    web::php5::mod { 'mysql': }

    # APACHE CONFIGURATION

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
    web::apache2::config { 'hcs-conf/directories.conf': }
    web::apache2::config { 'hcs-conf/redirects.conf': }
    web::apache2::config { 'hcs-conf/shibboleth.conf': }
    web::apache2::config { 'hcs-conf/tilde-rewrites.conf': }

    # HCS configurations for non-secure pages
    web::apache2::config { 'hcs-nonsecure-conf/redirects.conf': }

    # HCS configurations for secure pages
    web::apache2::config { 'hcs-ssl-conf/helios.conf': }
    web::apache2::config { 'hcs-ssl-conf/phpmyadmin.conf': }
    web::apache2::config { 'hcs-ssl-conf/rt.conf': }
    
    # Remove default vhost
    web::apache2::vhost{ '000-default':
      ensure => disabled,
    }

    # HCS enabled virtual hosts.
    #web::apache2::vhost{ 'default': }
    web::apache2::vhost{ 'hcs.harvard.edu': }
    web::apache2::vhost{ 'hcs.harvard.edu-ssl': }
    #web::apache2::vhost{ 'mail.hcs.harvard.edu': }
    #web::apache2::vhost{ 'secure.hcs.harvard.edu': }
    web::apache2::vhost{ 'user-vhosts': }

    # Mods enabled and disabled
    web::apache2::mod { 'actions': }
    web::apache2::mod { 'alias': }
    web::apache2::mod { 'authnz_ldap': }
    web::apache2::mod { 'cgi': }
    web::apache2::mod { 'dav': }
    web::apache2::mod { 'dav_fs': }
    web::apache2::mod { 'fcgid':
      require => Package['libapache2-mod-fcgid'],
    }
    web::apache2::mod { 'headers': }
    web::apache2::mod { 'include': }
    web::apache2::mod { 'ldap': }
    web::apache2::mod { 'php5':
      ensure  => disabled,
      require => Package['libapache2-mod-php5'],
    }
    web::apache2::mod { 'python':
      require => Package['libapache2-mod-python'],
    }
    web::apache2::mod { 'rewrite': }
    web::apache2::mod { 'shib2':
      require => Package['libapache2-mod-shib2'],
    }
    web::apache2::mod { 'ssl': }
    web::apache2::mod { 'suexec': }
    web::apache2::mod { 'suphp':
      require => Package['libapache2-mod-suphp'],
    }
    web::apache2::config { 'mods-available/userdir.conf': }
    web::apache2::mod { 'userdir': }
    web::apache2::mod { 'wsgi':
      require => Package['libapache2-mod-wsgi'],
    }

    # Symlink our web files to appropriate location
    file { '/var/www/hcs.harvard.edu':
      ensure  => link,
      target  => "${mount_dir}/www-hcs.harvard.edu",
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Must have mounted www-hcs.harvard.edu
      require => [Nfs::Client::Mount['www-hcs.harvard.edu'], Package['apache2']],
    }

    file { '/var/www/hcs.harvard.edu-ssl':
      ensure  => link,
      target  => "${mount_dir}/www-hcs.harvard.edu-ssl",
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Must have mounted www-hcs.harvard.edu-ssl
      require => [Nfs::Client::Mount['www-hcs.harvard.edu-ssl'], Package['apache2']],
    }
    
    @@haproxy::balancermember { "${::hostname}-web-http":
      listening_service => 'web-http',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['80'],
      define_cookies    => true,
      options           => ['check'],
    }
    
    @@haproxy::balancermember { "${::hostname}-web-https":
      listening_service => 'web-https',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['443'],
      define_cookies    => true,
      options           => ['check', 'ssl verify none'],
    }
  }
  elsif $::machine_type == 'lists' {
    require certs
    
    web::apache2::mod { 'alias': }
    web::apache2::mod { 'cgid': }
    web::apache2::mod { 'rewrite': }
    web::apache2::mod { 'ssl': }
    
    # Remove default vhost
    web::apache2::vhost{ '000-default':
      ensure => disabled,
    }
    web::apache2::vhost{ 'lists.hcs.harvard.edu': }
    web::apache2::vhost{ 'lists.hcs.harvard.edu-ssl': }
    
    @@haproxy::balancermember { "${::hostname}-lists-http":
      listening_service => 'lists-http',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['80'],
      define_cookies    => true,
      options           => ['check'],
    }
    
    @@haproxy::balancermember { "${::hostname}-lists-https":
      listening_service => 'lists-https',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['443'],
      define_cookies    => true,
      options           => ['check', 'ssl verify none'],
    }
  }
  
}
