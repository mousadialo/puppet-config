# apache2 configuration
# will install and configure apache2
# web servers will get more specific configuration
class apache2 {

  # These should be applied to all machines
  package { 'apache2':
    ensure => installed
  }

  service { 'apache2':
    ensure  => true,
    enable  => true,
    require => Package['apache2']
  }

  # FILES

  # Environment variables used by other config files
  apache2::config_file { 'envvars': }

  # This is the main apache configuratio file. It sets high level directives
  # and includes sites-enabled and conf.d
  apache2::config_file { 'apache2.conf': }

  # Which ports should listen on
  apache2::config_file { 'ports.conf': }

  # TODO: change this back to web!!!!
  if $::machine_type == 'generic' {

    /*
    file{ '/etc/apache2/sites-availible':
          ensure => directory,
          require => Package['apache2'],
    }
    */


    # Configurations shipped with Apache. We minimally edit these files.
    apache2::config_file { 'conf.d/charset': }
    apache2::config_file { 'conf.d/localized-error-pages': }
    apache2::config_file { 'conf.d/other-vhosts-access-log': }
    apache2::config_file { 'conf.d/security': }

    # HCS enabled virtual hosts.
    apache2::vhost{ '000-default': }
    apache2::vhost{ 'hcs.harvard.edu': }
    apache2::vhost{ 'hcs.harvard.edu-ssl': }
    apache2::vhost{ 'mail.hcs.harvard.edu': }
    apache2::vhost{ 'secure.hcs.harvard.edu': }
    apache2::vhost{ 'users-vhosts': }

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
    apache2::config_file { 'hcs-ssl-conf.d/phpmyadim.conf': }
    apache2::config_file { 'hcs-ssl-conf.d/rt.conf': }

    # PACKAGES

    # Apache modules
    package { 'libapache2-mod-php5': }
    package { 'libapache2-mod-suphp': }
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
    apache2::mod { 'auctions': }
    apache2::mod { 'authnz-ldap': }
    # TODO: WHAT IS THIS?
    #apache2::mod { 'authnz-ldap': }
    apache2::mod { 'cgi': }
    apache2::mod { 'dav': }
    apache2::mod { 'dav-fs':
      with_conf => true
    }
    apache2::mod { 'fcgid':
      with_conf => true
    }
    apache2::mod { 'headers': }
    apache2::mod { 'include': }
    apache2::mod { 'ldap': }
    apache2::mod { 'mime_magic':
      with_conf => true
    }
    apache2::mod { 'mod_python': }
    apache2::mod { 'perl': }
    apache2::mod { 'php5':
      with_conf => true
    }
    apache2::mod { 'proxy':
      with_conf => true
    }
    apache2::mod { 'proxy_ajp': }
    apache2::mod { 'proxy_balancer': }
    apache2::mod { 'proxy_http': }
    apache2::mod { 'rewrite': }
    apache2::mod { 'ssl':
      with_conf => true
    }
    apache2::mod { 'suexec': }
    apache2::mod { 'userdir': }

    # Remove default php confs
    # NOTE: this is probably bad.
    # we already tried to update the php5 load and conf files above...
/*    file { "/etc/apache2/mods-enabled/php5.load":
      ensure => absent
    }
    file { "/etc/apache2/mods-enabled/php5.conf":
      ensure => absent
    }
*/

    # Restart apache2 every time we make changes to this .ini file
    file {'/etc/php5/cgi/php.ini':
      ensure  => file,
      source  => 'puppet:///modules/apache2/php/php.ini',
      owner   => root,
      group   => root,
      notify  => Service['apache2'],
    }

    file {'/etc/php5/apache2/php.ini':
      ensure  => file,
      source  => 'puppet:///modules/apache2/php/apache2/php.ini',
      owner   => root,
      group   => root,
      notify  => Service['apache2']
    }
  }
}
