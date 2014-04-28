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

  service { 'idmapd':
    ensure => installed
  }

  service { 'nscd':
    ensure  => running,
    enable  => true,
    require => Package['nscd']
  }

  service { 'ssh':
    ensure    => running,
    enable    => true,
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

  # TODO: make this a separate module, potentially part of the base module
  file {'/etc/ssh/sshd_config':
    ensure => file,
    source => 'puppet:///modules/ldap/sshd_config',
    owner  => 'root',
    group  => 'root',
    mode   => '0644',
    notify => Service['ssh']
  }

  # This file is used for mapping user ids and group ids between filer and
  # clients. We put it here because mapping is done via ldap.
  file {'/etc/idmapd.conf':
    ensure  => file,
    content => template('ldap/idmapd.conf.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => File['/etc/nsswitch.conf'],
    notify  => Service['idmapd']
  }
}
