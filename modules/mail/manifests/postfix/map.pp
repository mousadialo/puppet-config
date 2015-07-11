# resource to postmap a file if changed
define mail::postfix::map () {

  mail::postfix::config { $title: } ~>
  exec { "/usr/sbin/postmap /etc/postfix/${title}":
    require => Package['postfix'],
    notify  => Service['postfix'],
  }

}
