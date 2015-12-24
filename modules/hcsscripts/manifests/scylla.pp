# Configures scylla libraries.
class hcsscripts::scylla {

  if $::machine_type == 'file' or $::machine_type == 'login' {    
    file { '/etc/hcs-scylla':
      ensure => directory,
      owner  => 'root',
      group  => 'root',
    }
  
    file { '/etc/hcs-scylla/ca.key':
      ensure  => file,
      content => hiera('scylla_ca.key'),
      owner   => 'root',
      group   => 'root',
      mode    => '0400',
      require => File['/etc/hcs-scylla'],
    }
    
    file { '/etc/hcs-scylla/ca.pem':
      ensure  => file,
      source  => 'puppet:///modules/hcsscripts/scylla/certs/ca.pem',
      owner   => 'root',
      group   => 'root',
      mode    => '0444',
      require => File['/etc/hcs-scylla'],
    }
  
    file { '/usr/lib/python2.7/scylla':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/hcsscripts/scylla/lib/scylla',
      owner   => 'root',
      group   => 'root',
    }
    
    file { '/usr/bin/new-cert':
      ensure  => file,
      source  => 'puppet:///modules/hcsscripts/scylla/bin/new-cert',
      owner   => 'root',
      group   => 'root',
      mode    => '0755',
      require => [
        File['/etc/hcs-scylla/ca.key'],
        File['/etc/hcs-scylla/ca.pem'],
        File['/usr/lib/python2.7/scylla'],
      ],
    }
    
    if $::machine_type == 'file' {
      file { '/etc/hcs/scylla_services':
        ensure  => directory,
        owner   => 'root',
        group   => 'root',
        require => File['/etc/hcs'],
      }
      
      exec { '/usr/bin/new-cert -s zfsquota':
        creates => '/etc/hcs/scylla_services/zfsquota_cert.pem',
        require => [
          File['/usr/bin/new-cert'],
          File['/etc/hcs/scylla_services'],
        ],
      }
    }
  }

}