# HCS Alpine configuration
class alpine {

  package { 'aspell':
    ensure => installed,
  } ->
  package { 'alpine':
    ensure => installed,
  } ->
  file { '/etc/pine.conf':
    ensure => present,
    source => 'puppet:///modules/alpine/pine.conf',
    owner  => 'root',
    group  => 'root',
  } ->
  file { '/usr/bin/pine':
    ensure => link,
    target => '/usr/bin/alpine',
  }

}
