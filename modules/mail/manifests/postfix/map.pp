# resource to postmap a file if changed
define mail::postfix::map () {

  mail::postfix::config { $title: } ~>
  exec { "/usr/sbin/postmap cdb:/etc/postfix/${title}":
    refreshonly => true,
    require     => [Package['postfix'], Package['postfix-cdb']],
    notify      => Service['postfix'],
  }

}
