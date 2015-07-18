# Userfacing packages available to all HCS users
class userfacing {

  # Alpine configuration
  package { 'aspell':
    ensure => installed,
  } ->
  package { 'alpine':
    ensure => installed,
  } ->
  file { '/etc/pine.conf':
    ensure => present,
    source => 'puppet:///modules/userfacing/alpine/pine.conf',
    owner  => 'root',
    group  => 'root',
  } ->
  file { '/usr/bin/pine':
    ensure => link,
    target => '/usr/bin/alpine',
  }
    
  @@haproxy::balancermember { "${::hostname}-login-ssh":
    listening_service => 'login-ssh',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['22'],
    options           => ['check'],
  }

  #$userfacing = hiera_array('userfacing_packages')
  #package { $userfacing:
  #  ensure => installed
  #}

}
