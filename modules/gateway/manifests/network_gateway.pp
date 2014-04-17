# this bundle is for being a gateway in a networking sense
class gateway::network_gateway {

  file { '/etc/network':
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => '0755'
  }

  file { '/etc/sysctl.d/50-ip_forward.conf':
    ensure => present,
    source => 'puppet:///modules/gateway/50-ip_forward.conf',
  }

  if $::machine_type == 'gateway' {
    file { 'gateway-interfaces' :
      ensure  => present,
      path    => '/etc/network/interfaces',
      source  => 'puppet:///modules/gateway/interfaces.G50_gateway-server',
      require => File['/etc/network']
    }
  }
  else {
    file { 'regular-interfaces' :
      ensure  => present,
      path    => '/etc/network/interfaces',
      source  => 'puppet:///modules/gateway/interfaces',
      require => File['/etc/network']
    }
  }

}
