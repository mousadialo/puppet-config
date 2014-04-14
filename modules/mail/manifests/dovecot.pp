# dovecot configuration for HCS mail servers
class mail::dovecot {

  package { 'dovecot-imapd' :
    ensure => installed
  }

  package { 'dovecot-pop3d' :
    ensure => installed
  }

  service { 'dovecot':
    ensure    => running,
    enable    => true,
    restart   => true,
    require   => [Package['dovecot-imapd'],Package['dovecot-pop3d']],
    subscribe => File['/etc/dovecot/dovecot.conf']
  }

  file { '/etc/dovecot/dovecot.conf' :
    ensure  => present,
    source  => 'puppet:///modules/mail/dovecot/dovecot.conf',
    notify  => Service['dovecot'],
    require => [Package['dovecot-imapd'],Package['dovecot-pop3d']]
  }
}
