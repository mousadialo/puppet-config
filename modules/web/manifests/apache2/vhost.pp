# resource to drop apache vhost files
define web::apache2::vhost () {

  file { "/etc/apache2/sites-available/${title}.conf":
    ensure  => file,
    content => template("web/vhosts/${title}.conf.erb"),
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package['apache2'],
  }

  file { "/etc/apache2/sites-enabled/${title}.conf":
    ensure  => link,
    target  => "/etc/apache2/sites-available/${title}.conf",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => [Package['apache2'], File["/etc/apache2/sites-available/${title}.conf"]],
  }

}
