# postfix config resource
define mail::postfix::config ($template = false) {

  if $template {
    file { "/etc/postfix/${title}":
      ensure  => file,
      content => template("mail/postfix/${title}.erb"),
      owner   => 'root',
      group   => 'root',
      require => Package['postfix'],
      notify  => Service['postfix'],
    }
  }
  else {
    file { "/etc/postfix/${title}":
      ensure  => file,
      source  => "puppet:///modules/mail/postfix/${title}",
      owner   => 'root',
      group   => 'root',
      require => Package['postfix'],
      notify  => Service['postfix'],
    }
  }
  
}
