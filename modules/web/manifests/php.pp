# PHP configuration
class web::php {
    
    #package { 'libapache2-mod-php5': }
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
    #file {'/etc/php5/apache2/php.ini':
    #  ensure  => file,
    #  source  => 'puppet:///modules/web/php5/apache2/php.ini',
    #  owner   => root,
    #  group   => root,
    #  notify  => Service['apache2'],
    #  require => Package['libapache2-mod-php5'],
    #}
    
    package { 'libapache2-mod-suphp':
      require => Package['apache2'],
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

    # PHP5 modules
    web::php5::mod { 'mcrypt': }
    web::php5::mod { 'gd': }
    web::php5::mod { 'mysql': }
    
}