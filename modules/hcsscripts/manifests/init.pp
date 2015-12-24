# Configures various HCS acctserv scripts.
class hcsscripts {

  file { '/etc/hcs':
    ensure => directory,
    owner  => 'root',
    group  => 'root',
  }

  include hcsscripts::makelist
  include hcsscripts::scylla
  include hcsscripts::zfsquota

}