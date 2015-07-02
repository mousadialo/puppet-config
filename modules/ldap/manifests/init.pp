# LDAP config for HCS servers
# includes config for HCS LDAP server and client config for all other servers
class ldap {
  
  package { 'ldap-utils':
    ensure => installed,
  }

  if $::machine_type == 'ldap' {
    include ldap::server
    
    exported_vars::set { 'ldap_server':
        value => "ldap://${::fqdn}"
    }
  }
  else {
    include ldap::client    
  }
  
  $ldap_servers = get_exported_var('', 'ldap_server', ["test"])
  
  file { '/etc/ldap/ldap.conf':
    ensure  => file,
    content => template('ldap/ldap.conf.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['ldap-utils'],
  }

}
