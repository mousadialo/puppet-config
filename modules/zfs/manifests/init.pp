# ZFS configuration for HCS file machine
# This configuration currently sets up the file zpool, not the clients
class zfs ($zpool_name = 'tank', $dataset_name = 'home') {
  include apt
  include nfs

  apt::ppa { 'ppa:zfs-native/stable': }
  ->
  package { 'ubuntu-zfs':
    ensure => latest,
  }
  ->
  zpool { $zpool_name:
   ensure => present,
   raidz  => ['xvdf', 'xvdg', 'xvdh', 'xvdi', 'xvdj']
  }
  ->
  zfs { "${zpool_name}/${dataset_name}":
    ensure     => present,
    canmount   => on,
    mountpoint => "/mnt/${zpool_name}/${dataset_name}",
    #sharenfs   => on, # turned this off because nfs will export it
    require    => Package['nfs-kernel-server']
  }
}
