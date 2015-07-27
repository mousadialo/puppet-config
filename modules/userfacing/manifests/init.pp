# Userfacing packages available to all HCS users
class userfacing {

  # install the base packages listed in data/hcs.yaml
  $userfacing = hiera_array('userfacing_packages')
  package { $userfacing:
    ensure => installed
  }
  
  # Alpine configuration
  file { '/etc/pine.conf':
    ensure  => present,
    source  => 'puppet:///modules/userfacing/alpine/pine.conf',
    owner   => 'root',
    group   => 'root',
    require => Package['alpine'],
  }
  
  file { '/usr/bin/pine':
    ensure  => link,
    target  => '/usr/bin/alpine',
    require => Package['alpine'],
  }
    
  @@haproxy::balancermember { "${::hostname}-login-ssh":
    listening_service => 'login-ssh',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['22'],
    options           => ['check'],
  }

}
