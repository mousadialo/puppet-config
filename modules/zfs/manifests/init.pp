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
    raidz  => ['xvdf', 'xvdg', 'xvdh', 'xvdi', 'xvdj']
  }
  ->
  zfs {'tank/home':
    ensure   => present,
    sharenfs => on
  }
}
