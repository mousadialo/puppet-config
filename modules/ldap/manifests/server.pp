# HCS LDAP server configuration
# NOTE: This class should not be used!
# Look at the private HCS ducks (docs) for instructions on setting up an LDAP server
class ldap::server {

  package { '389-ds-base':
    ensure => installed
  }

  #package { '389-ds-admin':
  #  ensure => installed
  #}

  #package { '389-ds-console':
  #  ensure => installed
  #}

  package { 'ldap-utils':
    ensure => installed
  }

  service { 'dirsrv':
    ensure  => running,
    enable  => true,
    require => Package['389-ds-base']
  }

  file {'/etc/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '644',
    notify  => Service['dirsrv'],
    require => Package['389-ds-base']
  }
  file {'/etc/dirsrv/schema/00core.ldif':
    ensure  => file,
    source  => 'puppet:///modules/ldap/00core.ldif',
    owner   => 'root',
    group   => 'root',
    mode    => '644',
    notify  => Service['dirsrv'],
    require => Package['389-ds-base']
  }

}
