# Installs and enables php5 modules.
define web::php5::mod($ensure = 'enabled', $package_name = "php5-${title}", $file_name = "20-${title}") {
  
  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")
  
  if $ensure == 'enabled' {
    package { $package_name:
      ensure  => installed,
      require => Package['php5-cgi'],
    }
    
    exec { "/usr/sbin/php5enmod ${title}" :
      unless  => "/bin/readlink -e /etc/php5/cgi/conf.d/${file_name}.ini 1> /dev/null",
      require => [Package['php5-cgi'], Package[$package_name]],
    }
  }
  else {
    exec { "/usr/sbin/php5dismod ${title}" :
      onlyif  => "/bin/readlink -e /etc/php5/cgi/conf.d/${file_name}.ini 1> /dev/null",
      require => Package['php5-cgi'],
    }
  }
  
}

