# postfix config resource
define mail::postfix::config () {

  file { "/etc/postfix/${title}":
    ensure  => file,
    source  => "puppet:///modules/mail/postfix/${title}",
    owner   => 'root',
    group   => 'root',
    require => Package['postfix'],
    notify  => Service['postfix'],
  }
  
}
