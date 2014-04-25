# function to postmap a file if changed
define mail::postmapfile ($name, $map=false) {
  include mail::postfix

  file { "/etc/postfix/${name}":
    ensure  => file,
    path    => "/etc/postfix/${name}",
    source  => "puppet:///modules/mail/postfix/${title}",
    owner   => 'root',
    group   => 'root',
    require => Package['postfix']
  }

  # we don't want to postmap everything, because some files are config files
  if $map == true {
    exec { "postmap${name}":
      command     => "/usr/sbin/postmap /etc/postfix/${name}",
      refreshonly =>  true,
      require     => [Package['postfix'], File["/etc/postfix/${name}"]],
      notify      => Service['postfix']
    }
  }
  notify {"map is ${map}"}
}
