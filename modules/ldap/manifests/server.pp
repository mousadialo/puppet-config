# HCS LDAP server configuration
# NOTE: This class should not be used!
# Look at the README for instructions on setting up an LDAP server
class ldap::server {

  package { '389-ds-base':
    ensure => installed,
  }
  
  file { '/etc/dirsrv/schema/00core.ldif':
    ensure  => file,
    source  => 'puppet:///modules/ldap/00core.ldif',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['389-ds-base'],
  }
  
  $hashed_root_dn_pwd = hiera('hashed_root_dn_pwd')
  $root_dn_pwd = hiera('root_dn_pwd')
  file { '/etc/dirsrv/config/setup.inf':
    ensure  => file,
    content => template('ldap/setup.inf.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0400',
    require => Package['389-ds-base'],
  }
  
  exec { 'setup-ds':
    command => '/usr/sbin/setup-ds --silent --file=/etc/dirsrv/config/setup.inf',
    creates => "/etc/dirsrv/slapd-${::hostname}",
    user    => 'root',
    require => File['/etc/dirsrv/config/setup.inf'],
  }
  
  service { 'dirsrv':
    ensure  => running,
    enable  => true,
    require => Exec['setup-ds'],
  }
  
}
