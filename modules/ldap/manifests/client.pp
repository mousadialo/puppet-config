# configuration for ldap client machines
class ldap::client {

  package { 'ldap-auth-client':
    ensure => installed
  }

  package { 'nscd':
    ensure => installed
  }

  service { 'nscd':
    ensure    => running,
    enable    => true,
    require   => Package['nscd']
    subscribe => File['/etc/ldap/ldap.conf'],
  }
  
  file { '/etc/ldap.conf':
    ensure  => link,
    target  => '/etc/ldap/ldap.conf',
    require => Package['ldap-auth-client']
  }

  file { '/etc/nsswitch.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/nsswitch.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    notify  => Service['nscd'],
    require => Package['nscd']
  }
}
