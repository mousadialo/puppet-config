# HCS LDAP server configuration
class ldap::server {

  package { '389-ds-base':
    ensure => installed
  }

  package { 'ldap-utils':
    ensure => installed
  }

  service { 'dirsrv':
    ensure  => running,
    enable  => true,
    require => Package['389-ds-base']
  }

}
