class hosts {
  require puppetdb

  # Remove ALL unmanaged host resources.
  resources { 'host':
    purge => true,
  }

  host { 'localhost':
    ensure       => present,
    host_aliases => $::fqdn,
    ip           => '127.0.0.1',
  }
  
  # The following hosts are desirable for IPv6 capable hosts
  host { 'ip6-localhost':
    ensure       => present,
    host_aliases => 'ip6-loopback',
    ip           => '::1',
  }
  host { 'ip6-localnet':
    ensure => present,
    ip     => 'fe00::0',
  }
  host { 'ip6-mcastprefix':
    ensure => present,
    ip     => 'ff00::0',
  }
  host { 'ip6-allnodes':
    ensure => present,
    ip     => 'ff02::1',
  }
  host { 'ip6-allrouters':
    ensure => present,
    ip     => 'ff02::2',
  }
  host { 'ip6-allhosts':
    ensure => present,
    ip     => 'ff02::3',
  }
  
}
