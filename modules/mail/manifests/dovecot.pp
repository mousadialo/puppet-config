# dovecot configuration for HCS mail servers
class mail::dovecot {

  require certs

  package { 'dovecot-imapd' :
    ensure => installed
  }

  package { 'dovecot-pop3d' :
    ensure => installed
  }

  file { '/etc/dovecot/dovecot.conf' :
    ensure  => present,
    source  => 'puppet:///modules/mail/dovecot/dovecot.conf',
    notify  => Service['dovecot'],
    require => [Package['dovecot-imapd'], Package['dovecot-pop3d']],
  } ~>
  service { 'dovecot':
    ensure  => running,
    enable  => true,
    restart => true,
    require => [Package['dovecot-imapd'], Package['dovecot-pop3d']],
  }
    
  @@haproxy::balancermember { "${::hostname}-mail-imaps":
    listening_service => 'mail-imaps',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['993'],
    options           => [],
  }
    
  @@haproxy::balancermember { "${::hostname}-mail-pop3s":
    listening_service => 'mail-pop3s',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['995'],
    options           => [],
  }

}
