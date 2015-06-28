class nfs::client::debian::service {
  Service {
    require => Class['nfs::client::debian::configure']
  }

  service { 'rpcbind':
    ensure    => running,
    enable    => true,
    hasstatus => false,
  }

  if $nfs::client::debian::nfs_v4 {
    service { 'idmapd':
      ensure    => running,
      name      => 'nfs-common',
      subscribe => Augeas['/etc/idmapd.conf', '/etc/default/nfs-common'],
    }
  } else {
    service { 'idmapd': ensure => stopped, }
  }
}
