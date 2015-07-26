# LDAP config for HCS servers
# includes config for HCS LDAP server and client config for all other servers
class ldap {
  
  package { 'ldap-utils':
    ensure => installed,
  }
  
  if $::machine_type == 'ldap' {
    include ldap::server
  }
  else {
    include ldap::client
  }

  file_line { 'ldap_base':
    path  => '/etc/ldap/ldap.conf',
    line  => "BASE\tdc=hcs,dc=harvard,dc=edu",
    match => '^#?BASE\s+\S+$',
  }

  $ldap_server = hiera('ldap-server')
  file_line { 'ldap_uri':
    path  => '/etc/ldap/ldap.conf',
    line  => "URI\tldaps://${ldap_server}",
    match => '^#?URI\s+\S+',
  }
  
  file_line { 'ldap_tls_reqcert':
    path  => '/etc/ldap/ldap.conf',
    line  => "TLS_REQCERT\tnever",
  }

}
