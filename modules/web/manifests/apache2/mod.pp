# resource to enable or disable apache2 mods
define web::apache2::mod ($ensure = 'enabled') {
  
  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")
  
  if $ensure == 'enabled' {
    exec { "/usr/sbin/a2enmod ${title}" :
      unless  => "/bin/readlink -e /etc/apache2/mods-enabled/${title}.load 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  else {
    exec { "/usr/sbin/a2dismod ${title}" :
      onlyif  => "/bin/readlink -e /etc/apache2/mods-enabled/${title}.load 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  
}

