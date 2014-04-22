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
  file {'/tank':
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => 644,
  }
  ->
  /* HACK Puppet provides the zpool resource. For whatever reason, when you try
   * to use it to create a zpool it fails because it thinks that our EBS volumes
   * are already in use. There is no force flag so we must execute the command
   * manually. */
  exec { 'create-zpool':
    command => "zpool create -f ${zpool_name} raidz xvdf xvdg xvdh xvdi xvdj",
    cwd => '/',
    logoutput => true,
    user => 'root',
    path => ["/sbin"],
    # Do not create the zpool if it already exists
    unless => "/sbin/zpool list | /bin/grep ${zpool_name} 2> /dev/null"
  }
  ->
  /* TODO Template this along with the zfs datasets */
  /* Correct permissions, owner, and group */
  file {"/${zpool_name}/home":
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => 644,
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
  /* Correct permissions, owner, and group */
  file {"/${zpool_name}/home/people":
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => 644,
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
  file {"/${zpool_name}/home/groups":
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => 644,
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
  file {"/${zpool_name}/home/general":
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => 644,
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
  file {"/${zpool_name}/home/hcs":
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => 644,
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
