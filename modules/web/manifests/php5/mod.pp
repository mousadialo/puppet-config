# Installs and enables php5 modules.
define web::php5::mod($ensure = 'enabled') {
  
  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")
  
  if $ensure == 'enabled' {
    package { "php5-${title}":
      ensure => installed,
      notify => Service['apache2'],
    }
    
    exec { "/usr/sbin/php5enmod ${title}" :
      unless  => "/bin/readlink -e /etc/php5/cgi/conf.d/20-${title}.ini 1> /dev/null",
      notify  => Service['apache2'],
      require => [Package['libapache2-mod-suphp'], Package["php5-${title}"]],
    }
  }
  else {
    exec { "/usr/sbin/php5dismod ${title}" :
      onlyif  => "/bin/readlink -e /etc/php5/cgi/conf.d/20-${title}.ini 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['libapache2-mod-suphp'],
    }
  }
  
}

