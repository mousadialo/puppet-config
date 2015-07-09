# Installs and enables php5 modules.
define apache2::php5_mod($ensure = 'enabled') {
  include apache2
  
  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")
  
  if $ensure == 'enabled' {
    package { "php5-${title}":
      ensure => installed,
      notify => Service[apache2],
    } ->
    exec { "/usr/sbin/php5enmod ${title}" :
      unless => "/bin/readlink -e /etc/php5/apache2/conf.d/20-${title}.ini 1> /dev/null",
      notify => Service[apache2],
    }
  }
  else {
    exec { "/usr/sbin/php5dismod ${title}" :
      onlyif => "/bin/readlink -e /etc/php5/apache2/conf.d/20-${title}.ini 1> /dev/null",
      notify => Service[apache2],
    }
  }
  
}

