# LDAP config for HCS servers
# includes config for HCS LDAP server and client config for all other servers
class ldap {
  
  $ldap_conf = '/etc/ldap/ldap.conf'
  
  if $::machine_type == 'ldap' {
    include ldap::server
  }
  else {
    include ldap::client    
  }

}
