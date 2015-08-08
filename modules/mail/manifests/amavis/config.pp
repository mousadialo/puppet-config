# amavis config resource
define mail::amavis::config ($template = false) {

  if $template {
    file { "/etc/amavis/conf.d/${title}":
      ensure  => file,
      content => template("mail/amavis/${title}.erb"),
      owner   => 'root',
      group   => 'root',
      require => Package['amavisd-new'],
      notify  => Service['amavis'],
    }
  }
  else {
    file { "/etc/amavis/conf.d/${title}":
      ensure  => file,
      source  => "puppet:///modules/mail/amavis/${title}",
      owner   => 'root',
      group   => 'root',
      require => Package['amavisd-new'],
      notify  => Service['amavis'],
    }
  }
  
}
