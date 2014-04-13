# function to drop postfix configuration files
define postfix::config_file () {
  include postfix

  file { $title:
    ensure  => file,
    path    => "/etc/postfix/${title}",
    source  => "puppet://modules/postfix/files/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Service['postfix'],
    require => Package['postfix']
  }

}
