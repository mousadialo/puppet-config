class hosts {

  # Remove ALL unmanaged host resources.
  resources { 'host':
    purge => true,
  }

  host { 'localhost':
    ensure => present,
    ip     => '127.0.0.1',
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
    ip     => 'fe00::0',
  }
  host { 'ip6-allnodes':
    ensure => present,
    ip     => 'fe02::1',
  }
  host { 'ip6-allrouters':
    ensure => present,
    ip     => 'fe02::1',
  }
  host { 'ip6-allhosts':
    ensure => present,
    ip     => 'fe02::1',
  }
  
  # Export host entry. More on exported resources: https://docs.puppetlabs.com/puppet/latest/reference/lang_exported.html
  @@host { $::hostname:
    ensure => present,
    ip     => $::ipaddress,
  }
  
  # Collect all host entries, including the one exported by the current machine.
  Host <<| |>>
  
}
