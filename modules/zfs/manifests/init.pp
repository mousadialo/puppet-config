# ZFS configuration for HCS file machine
# This configuration currently sets up the file zpool, not the clients
class zfs ($zpool_name = 'tank') {
  include apt
  include nfs

  apt::ppa { 'ppa:zfs-native/stable': }
  ->
  package { 'ubuntu-zfs':
    ensure => latest,
  }
  ->
  # Enable automatic sharing
  shellvar { 'ZFS_SHARE':
    ensure => present,
    target => '/etc/default/zfs',
    value  => 'yes',
    quoted => 'single',
  }
  ->
  # Correct permissions, owner, and group
  file { '/tank':
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => '0755',
  }
  ->
  # Our EBS instances already contain all the necsesary user data.
  exec { 'import-zpool':
    command   => "zpool import -f ${zpool_name}",
    cwd       => '/',
    logoutput => true,
    user      => 'root',
    path      => ['/sbin'],
    timeout   => 0, # this will take a while
    # Do not create the zpool if it already exists
    unless    => "/sbin/zpool list | /bin/grep ${zpool_name} 2> /dev/null"
  }
  ->
  exec { 'share-all':
    command   => 'zfs share -a',
    cwd       => '/',
    logoutput => true,
    user      => root,
    path      => ['/sbin'],
    timeout   => 0, # this will take a while as well
    # Do not share if export list is already populated
    unless    => "/sbin/showmount -e | /bin/grep ${zpool_name} 2> /dev/null"
  }
}
