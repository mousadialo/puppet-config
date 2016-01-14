# Apache2 configuration
class web::apache2 {

  $ssl_ciphers = hiera('ssl-ciphers')
    
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
  web::apache2::config { 'apache2': }
    
  # Security configurations to limit exposing ServerTokens and ServerSignature.
  web::apache2::config { 'security':
    ensure    => enabled,
    directory => 'conf-available/',
  }
    
  # Remove default vhost
  web::apache2::vhost{ 'default':
    ensure => disabled,
    order  => '000',
  }

  if $::machine_type == 'web' {
    require certs
    
    # PACKAGES
    
    # Apache modules
    package { 'apache2-suexec-custom':
      require => Package['apache2'],
    } ->
    web::apache2::config { 'www-data':
      directory => 'suexec/',
      extension => '',
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
    
    # Phusion Passenger (for Ruby on Rails and Node.js webapps)
    package { 'libapache2-mod-passenger':
      require => Package['apache2'],
    }

    # APACHE CONFIGURATION
    
    # These do spiffy HCS specific things like redirects for special people,
    # hosting from user directories and removing the tilde. These are applied to
    # secure and non-secure pages.
    file { '/etc/apache2/hcs-conf':
      ensure  => directory,
      recurse => true,
      purge   => true,
      source  => 'puppet:///modules/web/apache2/hcs-conf',
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    }

    # HCS configurations for non-secure pages
    file { '/etc/apache2/hcs-nonsecure-conf':
      ensure  => directory,
      recurse => true,
      purge   => true,
      source  => 'puppet:///modules/web/apache2/hcs-nonsecure-conf',
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    }

    # HCS configurations for secure pages
    file { '/etc/apache2/hcs-ssl-conf':
      ensure  => directory,
      recurse => true,
      purge   => true,
      source  => 'puppet:///modules/web/apache2/hcs-ssl-conf',
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
    
    # Virtual hosts enabled
    web::apache2::vhost{ 'www.hcs.harvard.edu':
      order => '000',
    }
    web::apache2::vhost{ 'mail.hcs.harvard.edu':
      order => '010',
    }
    web::apache2::vhost{ 'user_vhosts':
      order => '999',
    }

    # Mods enabled
    web::apache2::mod { 'actions': }
    web::apache2::mod { 'alias': }
    web::apache2::mod { 'asis': }
    web::apache2::mod { 'auth_basic': }
    web::apache2::mod { 'auth_form': }
    web::apache2::mod { 'authn_dbd': }
    web::apache2::mod { 'authn_dbm': }
    web::apache2::mod { 'authn_file': }
    web::apache2::mod { 'authnz_ldap': }
    web::apache2::mod { 'authz_dbd': }
    web::apache2::mod { 'authz_dbm': }
    web::apache2::mod { 'authz_groupfile': }
    web::apache2::mod { 'authz_host': }
    web::apache2::mod { 'authz_user': }
    web::apache2::mod { 'autoindex': }
    web::apache2::mod { 'cache': } ->
    web::apache2::mod { 'cache_disk': }
    web::apache2::mod { 'cgid': }
    web::apache2::mod { 'dav': }
    web::apache2::mod { 'dav_fs': }
    web::apache2::config { 'deflate':
      directory => 'mods-available/',
    } ->
    web::apache2::mod { 'deflate': }
    web::apache2::mod { 'dir': }
    web::apache2::mod { 'expires': }
    web::apache2::mod { 'fcgid':
      require => Package['libapache2-mod-fcgid'],
    }
    web::apache2::mod { 'filter': }
    web::apache2::mod { 'headers': }
    web::apache2::mod { 'include': }
    web::apache2::mod { 'ldap': }
    web::apache2::mod { 'mime': }
    web::apache2::mod { 'negotiation': }
    web::apache2::mod { 'passenger':
      require => Package['libapache2-mod-passenger'],
    }
    web::apache2::mod { 'python':
      require => Package['libapache2-mod-python'],
    }
    web::apache2::mod { 'rewrite': }
    web::apache2::mod { 'speling': }
    web::apache2::config { 'ssl':
      directory => 'mods-available/',
    } ->
    web::apache2::mod { 'ssl': }
    web::apache2::mod { 'status': }
    web::apache2::mod { 'suexec': }
    web::apache2::mod { 'unique_id': }
    web::apache2::config { 'userdir':
      directory => 'mods-available/',
    } ->
    web::apache2::mod { 'userdir': }
    web::apache2::mod { 'wsgi':
      require => Package['libapache2-mod-wsgi'],
    }
	
    # Add custom MIME types
	include web::mime
  }
  elsif $::machine_type == 'lists' {
    require certs
    
    # Virtual hosts enabled
    web::apache2::vhost{ 'lists.hcs.harvard.edu':
      order => '000',
    }
    
    # Mods enabled
    web::apache2::mod { 'alias': }
    web::apache2::mod { 'cache': } ->
    web::apache2::mod { 'cache_disk': }
    web::apache2::mod { 'cgid': }
    web::apache2::mod { 'headers': }
    web::apache2::mod { 'rewrite': }
    web::apache2::config { 'ssl':
      directory => 'mods-available/',
    } ->
    web::apache2::mod { 'ssl': }
  }
  
  # Apache2 module which enables the proxy protocol.
  # For more information and updates: https://github.com/ggrandes/apache24-modules/blob/master/mod_myfixip.c
  web::apache2::mod_source { 'myfixip': } ->
  web::apache2::mod { 'myfixip': }
  
  # Project Honeypot HTTP:BL implementation
  # See https://www.projecthoneypot.org/httpbl_download.php
  file { '/var/log/httpbl':
    ensure => directory,
    owner  => 'www-data',
    group  => 'www-data',
    mode   => '0755',
  } ->
  web::apache2::config { 'httpbl':
    directory => 'mods-available/',
  } ->
  web::apache2::mod_source { 'httpbl': } ->
  web::apache2::mod { 'httpbl': }
  
}