# resource to drop apache vhost files
define web::apache2::vhost ($ensure = 'enabled') {

  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")

  if $ensure == 'enabled' {
    $domain = hiera('domain')
    file { "/etc/apache2/sites-available/${title}.conf":
      ensure  => file,
      content => template("web/apache2/sites-available/${title}.conf.erb"),
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    } ->
    exec { "/usr/sbin/a2ensite ${title}" :
      unless  => "/bin/readlink -e /etc/apache2/sites-enabled/${title}.conf 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  else {
    exec { "/usr/sbin/a2dissite ${title}" :
      onlyif  => "/bin/readlink -e /etc/apache2/sites-enabled/${title}.conf 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }

}
