# HCS NFS configuration, used for ZFS setup
class nfs {

  $nfs_server = hiera('nfs-server')
  $zpool_name = hiera('zfs::zpool_name')
  $dataset_name = hiera('zfs::dataset_name')

  if $::machine_type == 'file' {
    package { 'nfs-kernel-server':
          ensure => 'installed'
    }
  }

  # Filer should not mount nfs
  if $::machine_type != 'file' {
    class { 'nfs::client':
      nfs_v4            => false,
      nfs_v4_mount_root => '/nfs'
    }
    nfs::client::mount {'nfs':
        server => "${nfs_server}",
        share  => "${zpool_name}/${dataset_name}",
        mount  => "/mnt/${zpool_name}/${dataset_name}",
        options => 'vers=3,default'
        atboot => true
    }
  }
}
