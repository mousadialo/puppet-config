class postfix {

  package { 'postfix':
    ensure => installed
  }

  package { 'postfix-cdb':
    ensure => installed
  }

  service { 'postfix':
    ensure => running,
    enable => true,
    require => Package['postfix'],
  }

  file { '/etc/postfix/main.cf':
    ensure => present,
    source => 'puppet:///modules/ntp/ntp.conf',
    notify => Service['postfix'],
    require => Package['postfix']
  }

}
