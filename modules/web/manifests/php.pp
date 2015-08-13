# PHP configuration
class web::php {
    
    package { 'libapache2-mod-suphp':
      require => Package['apache2'],
    }
    
    # Custom PHP configs. Changes include:
    # - Higher file upload size
    # - Use NFS session directory
    file {'/etc/php5/cgi/php.ini':
      ensure  => file,
      source  => 'puppet:///modules/web/php5/cgi/php.ini',
      owner   => root,
      group   => root,
      notify  => Service['apache2'],
      require => Package['libapache2-mod-suphp'],
    }
    
    if $::fqdn == hiera('php5-cron-server') {
      file { '/etc/cron.d/php5':
        ensure  => file,
        source  => 'puppet:///modules/web/php5/cron/php5',
        owner   => 'root',
        group   => 'root',
        mode    => '0644',
        require => Package['libapache2-mod-suphp'],
      }
    }
    else {
      file { '/etc/cron.d/php5':
        ensure  => absent,
        require => Package['libapache2-mod-suphp'],
      }
    }
    
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
    
    web::apache2::mod { 'suphp':
      require => Package['libapache2-mod-suphp'],
    }

    # PHP5 modules
    web::php5::mod { 'curl': }
    web::php5::mod { 'gd': }
    web::php5::mod { 'ldap': }
    web::php5::mod { 'mcrypt': }
    web::php5::mod { 'memcached': }
    web::php5::mod { 'mysql': }
    web::php5::mod { 'sqlite3':
      package_name => 'php5-sqlite',
    }
    web::php5::mod { 'tidy': }
    
    web::php5::mod { 'opcache':
      ensure    => disabled,
      file_name => '05-opcache',
    }
    
}