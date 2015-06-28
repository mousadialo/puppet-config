class nfs::server::redhat(
  $nfs_v4              = false,
  $nfs_v4_idmap_domain = undef,
  $mountd_port         = undef,
  $mountd_threads      = 1
) {

  if !defined(Class['nfs::client::redhat']) {
    class{ 'nfs::client::redhat':
      nfs_v4              => $nfs_v4,
      nfs_v4_idmap_domain => $nfs_v4_idmap_domain,
    }
  }

  if ($mountd_port != undef){
    fail('Setting mountd port currently not supported on RedHat')
  }

  include nfs::server::redhat::install, nfs::server::redhat::service


}
