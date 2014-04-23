# HCS NFS configuration, used for ZFS setup
class nfs ($nfs_home_directory = 'false' ) {
  /*
   * nfs_home_directory If true, the /home directory is mounted from the filer.
   */

  include base

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

    $mount_dir = hiera('nfs-mount-dir')

    class { 'nfs::client':
      nfs_v4            => false,
      nfs_v4_mount_root => '/nfs'
    }
    nfs::client::mount {'nfs':
        server => $nfs_server,
        share  => "/${zpool_name}/home",
        mount  => $mount_dir,
        options => 'vers=3,defaults',
        atboot => true
    }

    if str2bool($nfs_home_directory) {
      # We want to symlink our home directory to nfs
      file {"home":
        ensure  => link,
        path    => '/home',
        target  => $mount_dir,
        force   => 'true',
        owner   => 'root',
        group   => 'root',
        # 1) Must have mounted nfs
        # 2) Ubuntu user has a new, local home directory
        require => [Nfs::Client::Mount['nfs'], User['hcs']]
      }
    }
  }
}
