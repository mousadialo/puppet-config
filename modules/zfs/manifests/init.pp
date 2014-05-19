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
  # Correct permissions, owner, and group
  file {'/tank':
    ensure => directory,
    owner  => 'root',
    group  => 'root',
    mode   => '0644',
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
}
