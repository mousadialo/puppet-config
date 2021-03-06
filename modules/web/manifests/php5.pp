# PHP5 configuration
class web::php5 {

  package { 'php5-cgi':
    ensure => installed,
  }

  package { 'php5-cli':
    ensure => installed,
  }
  
  # Custom PHP configs. Changes include:
  # - Higher file upload size
  # - Use NFS session directory
  file { '/etc/php5/cgi/php.ini':
    ensure  => file,
    source  => "puppet:///modules/web/php5/cgi/php.ini.${::machine_type}",
    owner   => root,
    group   => root,
    require => Package['php5-cgi'],
  }
  
  file { '/etc/php5/cli/php.ini':
    ensure  => file,
    source  => "puppet:///modules/web/php5/cli/php.ini.${::machine_type}",
    owner   => root,
    group   => root,
    require => Package['php5-cli'],
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