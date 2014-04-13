# HCS NFS configuration, used for ZFS setup
class nfs {

  # Only the filer needs NFS server
  if $::machine_type == 'file' {
    class { 'nfs::server':
      nfs_v4 => true,
      nfs_v4_export_root_clients =>
        '10.0.0.0/16(rw,fsid=root,no_subtree_check,async,no_root_squash)'
    }

    nfs::server::export{ "/mnt/${zfs::zpool_name}":
      ensure  => 'mounted',
      clients => '10.0.0.0/16(rw,fsid=root,no_subtree_check,async,no_root_squash)',
      require  => Class['zfs']
    }
  }

  # Anything else that requires NFS will be using the filer as a client
  else {
    class { 'nfs::client':
      nfs_v4 => true,
      nfs_v4_mount_root => '/nfs'
    }
    Nfs::Client::Mount {'nfs':
        server => hiera('nfs-server'),
        share => 'tank',
        atboot => true
    }
  }
}
