# postfix configuration for HCS mail servers
class mail::postfix {

  package { 'postfix':
    ensure => installed,
  }

  package { 'postfix-cdb':
    ensure => installed,
  }

  service { 'postfix':
    ensure    => running,
    enable    => true,
    restart   => true,
    require   => Package['postfix'],
    subscribe => File['/etc/postfix/main.cf'],
  }

  if $::machine_type == 'mail' {
    # main.cf configuration
    mail::postfix::config { 'main.cf': }
    mail::postfix::config { 'master.cf': }
    mail::postfix::config { 'mynetworks': }
    mail::postfix::config { 'nobl_cidr': }

    # we want to ensure the following files are postmapped if modified
    # this means that we will refresh the postmap dbs automatically
    # the postfix::map resource also adds the file to the correct location
    mail::postfix::map { 'access': }
    mail::postfix::map { 'blacklist': }
    mail::postfix::map { 'canonical': }
    mail::postfix::map { 'nobl': }

    file { '/etc/aliases':
      ensure  => file,
      source  => 'puppet:///modules/mail/postfix/aliases',
      owner   => 'root',
      group   => 'root',
      require => Package['postfix'],
    } ~>
    # we want to ensure that we postalias the aliases files
    exec { '/usr/sbin/postalias /etc/aliases':
      refreshonly => true,
      notify      => Service['postfix'],
    }
  }
  else {
    # servers that aren't mail should use the NULL client configuration
    file { '/etc/postfix/main.cf':
      ensure  => file,
      source  => 'puppet:///modules/mail/postfix/main.cf.client',
      owner   => 'root',
      group   => 'root',
      require => Package['postfix'],
    }
  }

}
