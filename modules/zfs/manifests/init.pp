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
  /* Correct permissions, owner, and group */
  file {'/tank/home':
    ensure => directory,
    owner  => 'ubuntu',
    group  => 'ubuntu',
    mode   => 644,
  }
  ->
  /* HACK Puppet provides the zpool resource. For whatever reason, when you try
   * to use it to create a zpool it fails because it thinks that our EBS volumes
   * are already in use. There is no force flag so we must execute the command
   * manually. */
  exec { 'create-zpool'
    command => "zpool create -f ${zpool_name} raidz xvdf xvdg xvdh xvdi xvdj",
    user => 'root',
    # Do not create the zpool if it already exists
    unless => "zpool list | grep ${zpool_name} 2> /dev/null"
  }
  ->
  # TODO template this
  zfs { "${zpool_name}/home":
    ensure     => present,
    canmount   => on,
    mountpoint => "/${zpool_name}/home",
    sharenfs   => 'rw',
    require    => Package['nfs-kernel-server']
  }
  ->
  zfs { "${zpool_name}/home/people":
    ensure     => present,
    canmount   => on,
    mountpoint => "/${zpool_name}/home/people",
    sharenfs   => 'rw',
    require    => Package['nfs-kernel-server']
  }
  ->
  zfs { "${zpool_name}/home/groups":
    ensure     => present,
    canmount   => on,
    mountpoint => "/${zpool_name}/home/groups",
    sharenfs   => 'rw',
    require    => Package['nfs-kernel-server']
  }
  ->
  zfs { "${zpool_name}/home/general":
    ensure     => present,
    canmount   => on,
    mountpoint => "/${zpool_name}/home/general",
    sharenfs   => 'rw',
    require    => Package['nfs-kernel-server']
  }
  ->
  zfs { "${zpool_name}/home/hcs":
    ensure     => present,
    canmount   => on,
    mountpoint => "/${zpool_name}/home/hcs",
    sharenfs   => 'rw',
    require    => Package['nfs-kernel-server']
  }
}
