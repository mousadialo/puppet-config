# dovecot configuration for HCS mail servers
class mail::dovecot {

  require certs

  package { 'dovecot-imapd' :
    ensure => installed
  }

  package { 'dovecot-pop3d' :
    ensure => installed
  }

  file { '/etc/dovecot' :
    ensure  => directory,
    recurse => remote,
    source  => 'puppet:///modules/mail/dovecot',
    owner   => 'root',
    group   => 'root',
    notify  => Service['dovecot'],
    require => [Package['dovecot-imapd'], Package['dovecot-pop3d']],
  }
  
  service { 'dovecot':
    ensure  => running,
    enable  => true,
    require => [Package['dovecot-imapd'], Package['dovecot-pop3d']],
  }
    
  @@haproxy::balancermember { "${::hostname}-mail-imap":
    listening_service => 'mail-imap',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['143'],
    options           => ['check'],
  }
    
  @@haproxy::balancermember { "${::hostname}-mail-imaps":
    listening_service => 'mail-imaps',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['993'],
    options           => ['check'],
  }
    
  @@haproxy::balancermember { "${::hostname}-mail-pop3":
    listening_service => 'mail-pop3',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['110'],
    options           => ['check'],
  }
    
  @@haproxy::balancermember { "${::hostname}-mail-pop3s":
    listening_service => 'mail-pop3s',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['995'],
    options           => ['check'],
  }

}
