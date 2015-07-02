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

  service { 'nscd':
    ensure  => running,
    enable  => true,
    require => Package['nscd']
  }
  
  file {'/etc/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap-server.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['ldap-auth-client']
  }

  concat { $ldap::ldap_conf:
    ensure  => present,
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    warn    => true,
    require => Package['ldap-utils'],
  }
  
  concat::fragment { 'ldap_conf_head':
    target => $ldap_conf,
    source => 'puppet:///modules/ldap/ldap.conf.head',
    order  => '1',
  }
  
  Concat::Fragment <<| target == $ldap_conf |>>
  
  concat::fragment { 'ldap_conf_tail':
    target => $ldap_conf,
    source => 'puppet:///modules/ldap/ldap.conf.tail',
    order  => '3',
  }

  # This is distinct from the above file and is probably the more important one!
  # Don't mix the previous ldap.conf and this one!
  file {'/etc/ldap/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap.conf',
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
