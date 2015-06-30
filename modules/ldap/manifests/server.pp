# HCS LDAP server configuration
# NOTE: This class should not be used!
# Look at the README for instructions on setting up an LDAP server
class ldap::server {

  package { '389-ds-base':
    ensure => installed
  }

  service { 'dirsrv':
    ensure  => running,
    enable  => true,
    require => Package['389-ds-base']
  }

  file {'/etc/ldap.conf':
    ensure  => file,
    source  => 'puppet:///modules/ldap/ldap-server.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    notify  => Service['dirsrv'],
    require => Package['389-ds-base']
  }
  
  file {'/etc/dirsrv/schema/00core.ldif':
    ensure  => file,
    source  => 'puppet:///modules/ldap/00core.ldif',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    notify  => Service['dirsrv'],
    require => Package['389-ds-base']
  }

}
