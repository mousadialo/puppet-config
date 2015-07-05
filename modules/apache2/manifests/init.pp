# apache2 configuration
# will install and configure apache2
# web servers will get more specific configuration
class apache2 {

  require nfs
  $domain = hiera('domain')

  # These should be applied to all machines
  package { 'apache2':
    ensure => installed,
  } ->
  service { 'apache2':
    ensure => running,
    enable => true,
  }

  # FILES

  # This is the main apache configuration file. It sets high level directives
  # and includes sites-enabled and conf.d
  apache2::config_file { 'apache2.conf': }

  ## Which ports should listen on
  #apache2::config_file { 'ports.conf': }

  # Environment variables used by other config files
  apache2::config_file { 'envvars': }

  if $::machine_type == 'web' {

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
      content => template('apache2/shibboleth2.xml.erb'),
      owner   => 'root',
      group   => 'root',
      notify  => [Service['shibd'], Service['apache2']],
    } ->
    service { 'shibd':
      ensure  => running,
      enable  => true,
      require => Package['apache2']
    }

    # Following 2 packages needed for drupal
    package { ['php5-gd', 'php5-mysql']:
      ensure => installed
    }
    
    # Configurations shipped with Apache. We minimally edit these files.
    apache2::config_file { 'conf.d/charset': }
    apache2::config_file { 'conf.d/localized-error-pages': }
    apache2::config_file { 'conf.d/other-vhosts-access-log': }
    apache2::config_file { 'conf.d/security': }

    # HCS enabled virtual hosts.
    apache2::vhost{ 'default': }
    apache2::vhost{ 'hcs.harvard.edu':
      template => true,
    }
    apache2::vhost{ 'hcs.harvard.edu-ssl':
      template => true,
    }
    apache2::vhost{ 'mail.hcs.harvard.edu': }
    apache2::vhost{ 'secure.hcs.harvard.edu': }
    apache2::vhost{ 'user-vhosts': }

    # create the hcs conf directories
    file{ [ '/etc/apache2/hcs-conf.d',
            '/etc/apache2/hcs-nonsecure-conf.d',
            '/etc/apache2/hcs-ssl-conf.d' ]:
      ensure  => directory,
      require => Package['apache2'],
    }

    # These do spiffy HCS specific things like redirects for special people,
    # hosting from user directories and removing the tilde. These are applied to
    # secure and non-secure pages.
    apache2::config_file { 'hcs-conf.d/directories.conf': }
    apache2::config_file { 'hcs-conf.d/redirects.conf': }
    apache2::config_file { 'hcs-conf.d/tilde-rewrites.conf': }
    apache2::config_file { 'hcs-conf.d/userdir.conf': }

    # HCS configurations for non-secure pages
    apache2::config_file { 'hcs-nonsecure-conf.d/redirects.conf': }

    # HCS configurations for secure pages
    apache2::config_file { 'hcs-ssl-conf.d/phpmyadmin.conf': }
    apache2::config_file { 'hcs-ssl-conf.d/rt.conf': }

    # PACKAGES

    # Apache modules
    package { 'libapache2-mod-php5': }
    package { 'libapache2-mod-suphp': }

    # Custom suphp conf which disables checking the Document root. If we don't
    # do this it errors because we symlink our /var/www to
    # /mnt/tank/hcs.harvard.edu
    file {'/etc/suphp/suphp.conf':
      ensure  => file,
      source  => 'puppet:///modules/apache2/apache2/mod-config/suphp.conf',
      owner   => root,
      group   => root,
      notify  => Service['apache2'],
      require => Package['libapache2-mod-suphp']
    }

    package { 'libapache2-mod-fcgid': }

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

    # Mods enabled */
    apache2::mod { 'actions': }
    apache2::mod { 'authnz_ldap': }
    apache2::mod { 'cgi': }
    apache2::mod { 'dav': }
    apache2::mod { 'dav_fs':
      with_conf => true
    }
    apache2::mod { 'fcgid':
      with_conf => true
    }
    apache2::mod { 'headers': }
    apache2::mod { 'include': }
    apache2::mod { 'ldap': }
    apache2::mod { 'mod_python': }
    apache2::mod { 'proxy':
      with_conf => true
    }
    apache2::mod { 'proxy_ajp': }
    apache2::mod { 'proxy_balancer': }
    apache2::mod { 'proxy_http': }
    apache2::mod { 'rewrite': }
    apache2::mod { 'shib2': }
    apache2::mod { 'ssl':
      with_conf => true
    }
    apache2::mod { 'suexec': }
    apache2::mod { 'userdir': }
    apache2::mod { 'wsgi':
      with_conf => true
    }

    # Remove default php confs
    file { '/etc/apache2/mods-enabled/php5.load':
      ensure => absent
    }
    file { '/etc/apache2/mods-enabled/php5.conf':
      ensure => absent
    }

    # Restart apache2 every time we make changes to this .ini file
    file {'/etc/php5/cgi/php.ini':
      ensure  => file,
      source  => 'puppet:///modules/apache2/php/cgi/php.ini',
      owner   => root,
      group   => root,
      notify  => Service['apache2'],
      require => Package['libapache2-mod-php5']
    }

    file {'/etc/php5/apache2/php.ini':
      ensure  => file,
      source  => 'puppet:///modules/apache2/php/apache2/php.ini',
      owner   => root,
      group   => root,
      notify  => Service['apache2'],
      require => Package['libapache2-mod-php5']
    }

    # Certificates
    file {'/etc/ssl/certs/hcs_harvard_edu_cert.cer':
      ensure => file,
      source => 'puppet:///modules/apache2/apache2/hcs_harvard_edu_cert.cer',
      owner  => root,
      group  => root,
      mode   => '0644'
    }

    file {'/etc/ssl/certs/hcs_harvard_edu_interm.cer':
      ensure => file,
      source => 'puppet:///modules/apache2/apache2/hcs_harvard_edu_interm.cer',
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
      mode    => '0400'
    }

    # Symlink our web files to appropriate location
    file { '/var/www/hcs.harvard.edu':
      ensure  => link,
      target  => '/mnt/tank/www-hcs.harvard.edu',
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Must have mounted www-hcs.harvard.edu
      require => Nfs::Client::Mount['www-hcs.harvard.edu']
    }

    file { '/var/www/hcs.harvard.edu-ssl':
      ensure  => link,
      target  => '/mnt/tank/www-hcs.harvard.edu-ssl',
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Must have mounted www-hcs.harvard.edu-ssl
      require => Nfs::Client::Mount['www-hcs.harvard.edu-ssl']
    }
  
  }
  
}
