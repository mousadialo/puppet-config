# ZFS configuration for HCS file machine
# This configuration currently sets up the file zpool, not the clients
class zfs {
  include apt
  include nfs

  apt::ppa { 'ppa:zfs-native/stable': }
  ->
  package { 'ubuntu-zfs':
    ensure => latest,
  }
  ->
  zpool { 'tank':
   ensure => present,
   raidz  => ['xvdf', 'xvdg', 'xvdh', 'xvdi', 'xvdj']
  }
  ->
  zfs {'tank/home':
    ensure     => present,
    canmount   => on,
    mountpoint => '/mnt/tank/home',
    sharenfs   => on,
    require    => Package['nfs-kernel-server']
    #require   => Class['nfs']
  }
}
