# HCS Alpine configuration
class base::alpine {

  file { '/etc/pine.conf':
    ensure => present,
    source => 'puppet:///modules/base/alpine/pine.conf',
    owner  => 'root',
    group  => 'root'
  }

}
