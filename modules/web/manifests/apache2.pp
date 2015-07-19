# Apache2 configuration
class web::apache2 {
    
  package { 'apache2':
    ensure => installed,
  }
  
  service { 'apache2':
    ensure  => running,
    enable  => true,
    require => Package['apache2'],
  }
  
  package { 'apache2-dev':
    ensure => installed,
  }

  # This is the main apache configuration file. It sets high level directives
  # and includes sites-enabled and conf-enabled
  web::apache2::config { 'apache2.conf': }
    
  # Security configurations to limit exposing ServerTokens and ServerSignature.
  web::apache2::config { 'conf-available/security.conf': }

  if $::machine_type == 'web' {
    require certs
    
    # PACKAGES
    
    # Apache modules
    package { 'apache2-suexec-custom':
      require => Package['apache2'],
    }

    package { 'libapache2-mod-fcgid':
      require => Package['apache2'],
    }

    package { 'libapache2-mod-wsgi':
      require => Package['apache2'],
    }
    
    # Perl modules
    package { 'libapache-session-perl':
      require => Package['apache2'],
    }
    package { 'libapache2-mod-perl2':
      require => Package['apache2'],
    }
    package { 'libdbd-mysql-perl': }

    # Python packages
    package { 'libapache2-mod-python':
      require => Package['apache2'],
    }

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
    web::apache2::mod { 'python':
      require => Package['libapache2-mod-python'],
    }
    web::apache2::mod { 'rewrite': }
    web::apache2::mod { 'ssl': }
    web::apache2::mod { 'suexec': }
    web::apache2::config { 'mods-available/userdir.conf': }
    web::apache2::mod { 'userdir': }
    web::apache2::mod { 'wsgi':
      require => Package['libapache2-mod-wsgi'],
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
  }
  
  
  # Apache2 module which enables the proxy protocol.
  # For more information and updates: https://github.com/ggrandes/apache24-modules/blob/master/mod_myfixip.c
  file { '/usr/lib/apache2/modules/mod_myfixip.c':
    ensure  => file,
    source  => 'puppet:///modules/web/apache2/mods/mod_myfixip.c',
    owner   => root,
    group   => root,
    require => Package['apache2'],
  } ~>
  exec { '/usr/bin/apxs -i -a -c /usr/lib/apache2/modules/mod_myfixip.c':
    refreshonly => true,
    require     => Package['apache2-dev'],
    notify      => Service['apache2'],
  } ->
  web::apache2::mod { 'myfixip': }
  
}