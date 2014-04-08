# HCS NFS configuration, used for ZFS setup
class nfs {

  # Only the filer needs NFS server
  if $::machine_type == 'file' {
    class { 'nfs::server':
      nfs_v4 => true,
      nfs_v4_export_root_clients =>
        '10.0.0.0/16(rw,fsid=root,no_subtree_check,async,no_root_squash)'
    }

    nfs::server::export{ '/mnt/tank':
      ensure  => 'mounted',
      clients => '10.0.0.0/16(rw,fsid=root,no_subtree_check,async,no_root_squash)',
      require  => Class['zfs']
    }
  }

  # Anything else that requires NFS will be using the filer as a client
  else {
    class { 'nfs::server':
      nfs_v4 => true,
      nfs_v4_export_root_clients =>
        '10.0.0.0/16(rw,fsid=root,no_subtree_check,async,no_root_squash)'
    }
      Nfs::Client::Mount <<| |>>
  }
}
