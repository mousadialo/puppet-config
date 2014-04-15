# configuration for haproxy
class gateway::haproxy {

  package { 'haproxy':
    ensure => installed
  }

  service { 'haproxy':
    ensure  => running,
    enable  => true,
    require => Package['haproxy'],
  }

  # haproxy user, doesn't need a real shell
  user { 'haproxy':
    name    => 'haproxy',
    home    => '/var/www',
    shell   => '/bin/false',
    require => Package['haproxy']
  }


}
