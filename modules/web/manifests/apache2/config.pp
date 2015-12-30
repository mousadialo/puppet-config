# function to drop generic apache2 configuration files
define web::apache2::config ($ensure = 'none', $directory = '', $extension = '.conf') {

  validate_re($ensure, '^(enabled|disabled|none)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled', 'disabled' and 'none'.")

  if $ensure == 'none' {
    file { "/etc/apache2/${directory}${title}${extension}":
      ensure  => file,
      source  => "puppet:///modules/web/apache2/${directory}${title}${extension}",
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  elsif $ensure == 'enabled' {
    file { "/etc/apache2/${directory}${title}${extension}":
      ensure  => file,
      source  => "puppet:///modules/web/apache2/${directory}${title}${extension}",
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    } ->
    exec { "/usr/sbin/a2enconf ${title}" :
      unless  => "/bin/readlink -e /etc/apache2/conf-enabled/${title}${extension} 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  else {
    exec { "/usr/sbin/a2disconf ${title}" :
      onlyif  => "/bin/readlink -e /etc/apache2/conf-enabled/${title}${extension} 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }

}
