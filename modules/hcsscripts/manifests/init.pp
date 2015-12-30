# Configures various HCS acctserv scripts.
class hcsscripts {

  file { ['/etc/hcs', '/var/log/hcs']:
    ensure => directory,
    owner  => 'root',
    group  => 'root',
  }

  include hcsscripts::acctutils
  include hcsscripts::makelist
  include hcsscripts::reloadvhosts
  include hcsscripts::scylla
  include hcsscripts::zfsquota

}