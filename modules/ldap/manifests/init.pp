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

  # hcs.harvard.edu points to our bifrost load balancers, which will redirect the
  # request to any available LDAP servers.
  file_line { 'ldap_uri':
    path  => '/etc/ldap/ldap.conf',
    line  => "URI\tldap://hcs.harvard.edu",
    match => '^#?URI\s+\S+',
  }

}
