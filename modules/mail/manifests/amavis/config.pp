# amavis config resource
define mail::amavis::config () {

  file { "/etc/amavis/conf.d/${title}":
    ensure  => file,
    source  => "puppet:///modules/mail/amavis/${title}",
    owner   => 'root',
    group   => 'root',
    require => Package['amavisd-new'],
    notify  => Service['amavis'],
  }
  
}
