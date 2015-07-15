# postfix config resource
define mail::postfix::config ($suffix = '', $template = false) {

  if $template {
    file { "/etc/postfix/${title}":
      ensure  => file,
      content => template("mail/postfix/${title}${suffix}.erb"),
      owner   => 'root',
      group   => 'root',
      require => Package['postfix'],
      notify  => Service['postfix'],
    }
  }
  else {
    file { "/etc/postfix/${title}":
      ensure  => file,
      source  => "puppet:///modules/mail/postfix/${title}${suffix}",
      owner   => 'root',
      group   => 'root',
      require => Package['postfix'],
      notify  => Service['postfix'],
    }
  }
  
}
