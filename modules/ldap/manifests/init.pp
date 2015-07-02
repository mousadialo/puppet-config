# LDAP config for HCS servers
# includes config for HCS LDAP server and client config for all other servers
class ldap {
  
  $ldap_conf = '/etc/ldap/ldap.conf'
  
  package { 'ldap-utils':
    ensure => installed,
  }

  concat { $ldap_conf:
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
  
  if $::machine_type == 'ldap' {
    include ldap::server
    
    @@concat::fragment { "ldap_conf_${::hostname}":
      target  => $ldap_conf,
      content => "ldap://${::fqdn}\t",
      order   => '2',
    }
  }
  else {
    include ldap::client    
  }
  
  Concat::Fragment <<| target == $ldap_conf |>>
  
  concat::fragment { 'ldap_conf_tail':
    target => $ldap_conf,
    source => 'puppet:///modules/ldap/ldap.conf.tail',
    order  => '3',
  }

}
