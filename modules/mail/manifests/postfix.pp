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
  #file { '/etc/postfix/main.cf':
    #ensure  => present,
    #source  => 'puppet:///modules/mail/postfix/main.cf',
    #notify  => Service['postfix'],
    #require => Package['postfix']
  #}
  mail::postmapfile { 'main.cf':    name => 'main.cf' }
  mail::postmapfile { 'master.cf':  name => 'master.cf' }
  mail::postmapfile { 'mynetworks': name => 'mynetworks' }
  mail::postmapfile { 'nobl_cidr':  name => 'nobl_cidr' }
  # master.cf configuration
  #file { '/etc/postfix/master.cf':
    #ensure  => present,
    #source  => 'puppet:///modules/mail/postfix/master.cf',
    #notify  => Service['postfix'],
    #require => Package['postfix']
  #}

  #file { '/etc/postfix/mynetworks':
    #ensure  => present,
    #source  => 'puppet:///modules/mail/postfix/mynetworks',
    #notify  => Service['postfix'],
    #require => Package['postfix']
  #}

  #file { '/etc/postfix/nobl_cidr':
    #ensure  => present,
    #source  => 'puppet:///modules/mail/postfix/nobl_cidr',
    #notify  => Service['postfix'],
    #require => Package['postfix']
  #}

  # we want to ensure the following files are postmapped if modified
  # this means that we will refresh the postmap dbs automatically
  # the postmapfile command also adds the file to the correct location
  mail::postmapfile { 'access':    name => access,    map => true }
  mail::postmapfile { 'blacklist': name => blacklist, map => true }
  mail::postmapfile { 'canonical': name => canonical, map => true }
  mail::postmapfile { 'nobl':      name => nobl,      map => true }

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
