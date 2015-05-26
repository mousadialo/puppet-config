# LDAP config for HCS servers
# includes config for HCS LDAP server and client config for all other servers
class ldap {

  host { 'ldap.hcs.so':
    ensure => present,
    ip     => hiera('ldap-server')
  }

  if $::machine_type == 'ldap' {
    include ldap::server
  }
  else {
    include ldap::client
  }


}
