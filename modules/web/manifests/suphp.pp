# suPHP configuration
class web::suphp {
    
    require web::php5
    
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
    
    web::apache2::mod { 'suphp':
      require => Package['libapache2-mod-suphp'],
    }
    
}