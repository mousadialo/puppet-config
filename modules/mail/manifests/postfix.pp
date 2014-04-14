# postfix configuration for HCS mail servers
class mail::postfix {

  package { 'postfix':
    ensure => installed
  }

  package { 'postfix-cdb':
    ensure => installed
  }

  service { 'postfix':
    ensure    => running,
    enable    => true,
    restart   => true,
    require   => Package['postfix'],
    subscribe => File['/etc/postfix/main.cf']
  }

  # main.cf configuration
  file { '/etc/postfix/main.cf':
    ensure  => present,
    source  => 'puppet:///modules/mail/postfix/main.cf',
    notify  => Service['postfix'],
    require => Package['postfix']
  }

  # master.cf configuration
  file { '/etc/postfix/master.cf':
    ensure  => present,
    source  => 'puppet:///modules/mail/postfix/master.cf',
    notify  => Service['postfix'],
    require => Package['postfix']
  }

  file { '/etc/postfix/nobl_cidr':
    ensure  => present,
    source  => 'puppet:///modules/mail/postfix/nobl_cidr',
    notify  => Service['postfix'],
    require => Package['postfix']
  }

  # we want to ensure the following files are postmapped if modified
  # this means that we will refresh the postmap dbs automatically
  # the postmapfile command also adds the file to the correct location
  mail::postmapfile { 'access': name => access }
  mail::postmapfile { 'blacklist': name => blacklist }
  mail::postmapfile { 'canonical': name => canonical }
  mail::postmapfile { 'nobl': name => nobl }

  # we want to ensure that we postalias the aliases files
  exec { 'postalias_aliases':
    command     => '/usr/sbin/postalias /etc/aliases',
    refreshonly =>  true,
    require     => [Package['postfix'], File['/etc/aliases']],
    notify      => Service['postfix']
  }

  file { '/etc/aliases':
    ensure  => file,
    path    => '/etc/aliases',
    source  => 'puppet:///modules/mail/postfix/aliases',
    owner   => 'root',
    group   => 'root',
    notify  => Exec['postalias_aliases'],
    require => Package['postfix']
  }


}
