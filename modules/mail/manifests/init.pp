# postfix configuration for HCS mail servers
class mail {

  # all machines will configure postfix
  include mail::postfix

  if $::machine_type == 'mail' {
    include mail::amavis
    include mail::dovecot
    
    package { 'procmail':
      ensure => installed,
    }
    
    packge { 'postgrey':
      ensure => installed,
    } ->
    shellvar { 'POSTGREY_OPTS':
      ensure => present,
      target => '/etc/default/postgrey',
      value  => '--inet=10023 --delay=60',
      quoted => 'double',
    } ~>
    service { 'postgrey':
      ensure => running,
      enable => true,
    }
    
    package { 'rt4-clients':
      ensure => installed,
    } ->
    file { '/etc/request-tracker4':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/mail/request-tracker4',
      owner   => 'root',
      group   => 'root',
    }
  }

}
