$mount_dir = hiera('nfs-mount-dir')

include nfs

# We want to symlink our home directory to nfs
file {"home":
  ensure  => link,
  path    => '/home',
  target  => $mount_dir,
  owner   => 'ubuntu',
  group   => 'ubuntu',
  # Must have mounted it
  require => Nfs::Client::Mount['nfs']
}

