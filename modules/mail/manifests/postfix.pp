# postfix configuration for HCS mail servers
class mail::postfix {

  package { 'postfix':
    ensure => installed,
  }

  package { 'postfix-cdb':
    ensure => installed,
  }

  service { 'postfix':
    ensure  => running,
    enable  => true,
    require => Package['postfix'],
  }

  if $::machine_type == 'mail' {
    require nfs
  
    # main.cf configuration
    mail::postfix::config { 'main.cf':
      suffix => '.mail',
    }
    mail::postfix::config { 'master.cf':
      template => true,
    }
    mail::postfix::config { 'mynetworks': }

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
    exec { '/usr/sbin/postalias cdb:/etc/aliases':
      refreshonly => true,
      require     => [Package['postfix'], Package['postfix-cdb']],
      notify      => Service['postfix'],
    }
    
    @@haproxy::balancermember { "${::hostname}-mail-smtp":
      listening_service => 'mail-smtp',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['25'],
      options           => ['send-proxy', 'check'],
    }
    
    @@haproxy::balancermember { "${::hostname}-mail-smtp-relay":
      listening_service => 'mail-smtp-relay',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['10025'],
      options           => ['check'],
    }
  }
  elsif $::machine_type == 'lists' {
    mail::postfix::config { 'main.cf':
      suffix => '.lists',
    }
    mail::postfix::config { 'mynetworks': }
    
    @@haproxy::balancermember { "${::hostname}-lists-smtp":
      listening_service => 'lists-smtp',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['25'],
      options           => ['check'],
    }
  }
  else {
    # servers that aren't mail should use the NULL client configuration
    mail::postfix::config { 'main.cf':
      suffix => '.client',
    }
  }

}
