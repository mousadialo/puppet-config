define apache2::config_file () {
  include apache2

  file {"/etc/apache2/{$title}":
    ensure  => file,
    path    => "/etc/apache2/$title",
    source  => "puppet:///modules/apache2/apache2/$title",
    owner   => root,
    group   => root,
    notify  => Service['apache2'],
    require => Package['apache2']
  }
}
