# Debian specifix stuff
class nfs::server::debian(
  $nfs_v4              = false,
  $nfs_v4_idmap_domain = undef,
  $mountd_port         = undef,
  $mountd_threads      = 1
) {

  if !defined(Class['nfs::client::debian']) {
    class{ 'nfs::client::debian':
      nfs_v4              => $nfs_v4,
      nfs_v4_idmap_domain => $nfs_v4_idmap_domain,
    }
  }

  if ($mountd_port != undef){
    shellvar { 'rpc-mount-options':
      ensure   => present,
      target   => '/etc/default/nfs-kernel-server',
      variable => 'RPCMOUNTDOPTS',
      value    => "--manage-gids --port ${mountd_port} --num-threads ${mountd_threads}",
      notify   => Service['nfs-kernel-server'],
    }
  }

  include nfs::server::debian::install, nfs::server::debian::service
}
