# resource to drop apache vhost files
define web::apache2::vhost ($ensure = 'enabled', $order = '000') {

  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")

  if $ensure == 'enabled' {
    $secondary_domains = hiera_array('secondary-domains')
    file { "/etc/apache2/sites-available/${order}-${title}.conf":
      ensure  => file,
      content => template("web/apache2/sites-available/${title}.conf.erb"),
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    } ->
    exec { "/usr/sbin/a2ensite ${order}-${title}" :
      unless  => "/bin/readlink -e /etc/apache2/sites-enabled/${order}-${title}.conf 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  else {
    exec { "/usr/sbin/a2dissite ${order}-${title}" :
      onlyif  => "/bin/readlink -e /etc/apache2/sites-enabled/${order}-${title}.conf 1> /dev/null",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }

}
