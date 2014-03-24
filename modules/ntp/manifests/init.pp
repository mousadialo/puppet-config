class ntp {

  package { 'ntp':
    ensure => installed
  }

  service { 'ntpd':
    ensure => ensure,
    enable => true,
    require => Package['ntp'],
    name => 'ntp'
  }

  file { '/etc/ntp.conf':
    ensure => present,
    source => 'puppet:///modules/ntp/ntp.conf',
    notify => Service['ntpd'],
    require => Package['ntp']
  }

}
