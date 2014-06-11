# configuration for ldap client machines
class ldap::client {
  $ldap_server = hiera('ldap-server')

  package { 'ldap-auth-client':
    ensure => installed
  }

  package { 'nscd':
    ensure => installed
  }

  package { 'ldap-utils':
    ensure => installed
  }

  service { 'nscd':
    ensure  => running,
    enable  => true,
    require => Package['nscd']
  }

  file {'/etc/ldap.conf':
    ensure  => file,
    content => template('ldap/ldap-server.conf.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['ldap-auth-client']
  }

  # This is distinct from the above file and is probably the more important one!
  # Don't mix the previous ldap.conf and this one!
  file {'/etc/ldap/ldap.conf':
    ensure  => file,
    content => template('ldap/ldap.conf.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['ldap-auth-client']
  }

  file {'/etc/nsswitch.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/nsswitch.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    notify  => Service['nscd'],
    require => Package['nscd']
  }
}
