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
  
  host { $::fqdn:
    ensure => present,
    ip     => $::ipaddress,
  }
  
  # Workaround to delay puppet from evaluating the actual host resource.
  # More info here: https://groups.google.com/forum/#!topic/puppet-users/Ux28jzVpteU
  define exported_host($ensure, $ip) {
    host { $title:
      ensure => $ensure,
      ip     => $ip,
    }
  }
  # Export host resource for other machines to use.
  # More on exported resources: https://docs.puppetlabs.com/puppet/latest/reference/lang_exported.html
  @@exported_host { $::fqdn:
    ensure => present,
    ip     => $::ipaddress,
  }
  # Collect other machines' exported resources.
  Exported_host <<| title != $::fqdn |>>
  
}
