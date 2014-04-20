# configuration for ldap client machines
class ldap::client {

  package { 'ldap-auth-client':
    ensure => installed
  }

  package { 'nscd':
    ensure => installed
  }

  package { 'ldap-utils':
    ensure => installed
  }

  # I think we want to ignore this...
  #service { 'libnss-ldap':
  #  ensure => stopped,
  #  enable => true,
  #}

  service { 'nscd':
    ensure  => running,
    enable  => true,
    require => Package['nscd']
  }

  service { 'sshd':
    ensure    => running,
    enable    => true,
  }

  file {'/etc/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['ldap-auth-client']
  }

  # This is distinct from the above file and is probably the more important one!
  # Don't mix the previous ldap.conf and this one!
  file {'/etc/ldap/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap-server/ldap.conf',
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
    notify => Service['sshd']
  }

}
