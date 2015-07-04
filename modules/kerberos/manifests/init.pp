class kerberos {

  package { 'krb5-user':
    ensure => installed,
  }

  package { 'krb5-config':
    ensure => installed,
  } ->
  file { '/etc/krb5.conf':
    ensure => file,
    source => 'puppet:///modules/kerberos/krb5.conf',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
  }

  package { 'libpam-krb5':
    ensure => installed,
  }

}