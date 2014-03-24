class apache2 {

  package { 'apache2':
    ensure => installed
  }

  service { 'apache2':
    ensure => true,
    enable => true,
    require => Package['apache2']
  }

  file { '/etc/apache2/httpd.conf':
    ensure => file,
    source => 'puppet:///modules/apache2/httpd.conf',
    notify => Service['apache2'],
    require => Package['apache2']
  }

  file { '/etc/apache2/hcs-conf.d':
    ensure => directory,
    source => 'puppet:///modules/apache2/hcs-conf.d',
    recurse => true,
    notify => Service['apache2'],
    require => Package['apache2']
  }
}
