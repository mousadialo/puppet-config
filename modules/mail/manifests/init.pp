# postfix configuration for HCS mail servers
class mail {

  # all machines will configure postfix
  include mail::postfix

  if $::machine_type == 'mail' {
    include mail::dovecot
    include mail::amavis
    
    # This is needed because groups will get messed up otherwise for some reason.
    Class['mail::dovecot'] -> Class['mail::amavis']
    
    package { 'procmail':
      ensure => installed,
    } ->
    file { '/etc/procmailrc':
      ensure => file,
      source => 'puppet:///modules/mail/procmail/procmailrc',
      owner  => 'root',
      group  => 'root',
      mode   => '0644',
    }
    
    package { 'postgrey':
      ensure => installed,
    } ->
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
