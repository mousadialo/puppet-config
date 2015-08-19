# Configures various HCS acctserv scripts.
class scripts {

  if $::machine_type == lists {
    package { 'python-twisted-web':
      ensure => installed,
    }
    
    package { 'python-crypto':
      ensure => installed,
    }
    
    file { '/usr/lib/makelist':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/scripts/makelist',
      owner   => 'root',
      group   => 'root',
      notify  => Service['hcs-makelist'],
    }
    
    file { '/etc/init.d/hcs-makelist':
      ensure => file,
      source => 'puppet:///modules/scripts/hcs-makelist',
      owner  => 'root',
      group  => 'root',
      mode   => '0755',
      notify  => Service['hcs-makelist'],
    }
    
    service { 'hcs-makelist':
      ensure  => running,
      enable  => true,
      status  => '/bin/ps auxww | /bin/grep hcs-makelist | /bin/grep -v grep 1> /dev/null',
      require => [
        Package['python-twisted-web'],
        Package['python-crypto'],
        File['/usr/lib/makelist'],
        File['/etc/init.d/hcs-makelist'],
      ]
    }
    
    @@haproxy::balancermember { "${::hostname}-makelist":
      listening_service => 'makelist',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['8080'],
      options           => ['check'],
    }
  }

}