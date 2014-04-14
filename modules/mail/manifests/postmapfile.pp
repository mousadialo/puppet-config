# function to postmap a file if changed
define mail::postmapfile ($name) {
  include mail::postfix

  exec { "postmap${name}":
    command     => "/usr/sbin/postmap /etc/postfix/${name}",
    refreshonly =>  true,
    require     => [Package['postfix'], File["/etc/postfix/${name}"]],
    notify      => Service['postfix']
  }

  file { "/etc/postfix/${name}":
    ensure  => file,
    path    => "/etc/postfix/${name}",
    source  => "puppet:///modules/postfix/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Exec["postmap${name}"],
    require => Package['postfix']
  }

}
