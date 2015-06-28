class nfs::server::redhat::service {
  if $::nfs::client::redhat::params::osmajor >= 7 {
    $service_name = 'nfs-server'
  } else {
    $service_name = 'nfs'
  }

  if $nfs::server::redhat::nfs_v4 == true {
    service {$service_name:
      ensure     => running,
      enable     => true,
      hasrestart => true,
      hasstatus  => true,
      require    => Package['nfs-utils'],
      subscribe  => [ Concat['/etc/exports'], Augeas['/etc/idmapd.conf'] ],
    }
  } else {
    service {$service_name:
      ensure     => running,
      enable     => true,
      hasrestart => true,
      hasstatus  => true,
      require    => Package['nfs-utils'],
      subscribe  => Concat['/etc/exports'],
    }
  }
}
