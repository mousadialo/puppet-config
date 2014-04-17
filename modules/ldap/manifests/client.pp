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

  service { 'ncsd':
    ensure  => running,
    enable  => true,
    require => Package['nscd']
  }

  file {'/etc/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap.conf',
    #owner  => 'root',
    #group  => 'root',
    notify  => Package['ldap-auth-client'],
    require => Package['ldap-auth-client']
  }

  file {'/etc/ldap/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap/ldap.conf',
    #owner  => 'root',
    #group  => 'root',
    notify  => Package['ldap-auth-client'],
    require => Package['ldap-auth-client']
  }

  file {'/etc/nsswitch.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/nsswitch.conf',
    #owner  => 'root',
    #group  => 'root',
    notify  => Service['nscd'],
    require => Package['nscd']
  }

}
