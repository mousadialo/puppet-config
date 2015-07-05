# function to postmap a file if changed
define mail::postmapfile ($name, $map=false) {
  include mail::postfix


  if $map {

    file { "/etc/postfix/${name}":
      ensure  => file,
      path    => "/etc/postfix/${name}",
      source  => "puppet:///modules/mail/postfix/${title}",
      owner   => 'root',
      group   => 'root',
      notify  => Exec["postmap${name}"],
      require => Package['postfix']
    }

    exec { "postmap${name}":
      command => "/usr/sbin/postmap /etc/postfix/${name}",
      require => [Package['postfix'], File["/etc/postfix/${name}"]],
      notify  => Service['postfix']
    }

  } else {

    file { "/etc/postfix/${name}":
      ensure  => file,
      path    => "/etc/postfix/${name}",
      source  => "puppet:///modules/mail/postfix/${title}",
      owner   => 'root',
      group   => 'root',
      require => Package['postfix']
    }

  }
}
