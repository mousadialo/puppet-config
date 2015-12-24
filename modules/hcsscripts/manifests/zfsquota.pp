# Configures zfsquota server script.
class hcsscripts::zfsquota {

  if $::machine_type == 'file' {
    require hcsscripts::scylla
    
    package { 'python-twisted-web':
      ensure => installed,
    }
    
    file { '/sbin/safezfs':
      ensure => file,
      source => 'puppet:///modules/hcsscripts/zfsquota/bin/safezfs',
      owner  => 'root',
      group  => 'root',
      mode   => '0755',
      notify => Service['hcs-zfsquota'],
    }
    
    file { '/usr/lib/zfsquota':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/hcsscripts/zfsquota/lib/zfsquota-server',
      owner   => 'root',
      group   => 'root',
      notify  => Service['hcs-zfsquota'],
    }
    
    file { '/etc/init.d/hcs-zfsquota':
      ensure => file,
      source => 'puppet:///modules/hcsscripts/zfsquota/init/hcs-zfsquota',
      owner  => 'root',
      group  => 'root',
      mode   => '0755',
      notify => Service['hcs-zfsquota'],
    }
    
    service { 'hcs-zfsquota':
      ensure  => running,
      enable  => true,
      status  => '/bin/ps auxww | /bin/grep hcs-zfsquota | /bin/grep -v grep 1> /dev/null',
      require => [
        Package['python-twisted-web'],
        File['/sbin/safezfs'],
        File['/usr/lib/zfsquota'],
        File['/etc/init.d/hcs-zfsquota'],
      ]
    }
    
    @@haproxy::balancermember { "${::hostname}-zfsquota":
      listening_service => 'zfsquota',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['8081'],
      options           => ['check'],
    }
  }
  elsif $::machine_type == 'login' {
    require hcsscripts::scylla
    
    file { '/usr/lib/python2.7/zfsquota':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/hcsscripts/zfsquota/lib/zfsquota-client',
      owner   => 'root',
      group   => 'root',
    }
    
    file { '/usr/bin/hcs-quota':
      ensure  => file,
      source  => 'puppet:///modules/hcsscripts/zfsquota/bin/hcs-quota',
      owner   => 'root',
      group   => 'root',
      mode    => '0755',
      require => File['/usr/lib/python2.7/zfsquota'],
    }
  }

}