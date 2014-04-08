# ZFS configuration for HCS file machine
# This configuration currently sets up the file zpool, not the clients
class zfs {
  include apt

  apt::ppa { 'ppa:zfs-native/stable': }
  ->
  package { 'ubuntu-zfs':
    ensure => latest,
  }
  ->
  zpool { 'tank':
    ensure => present
    raidz  => ['xvdf xvdg xvdh xvdi xvdj']
  }
  ->
  zfs {'/mnt/tank/home':
    ensure   => present,
    sharenfs => on,
    require  => Class['nfs']
  }
}
