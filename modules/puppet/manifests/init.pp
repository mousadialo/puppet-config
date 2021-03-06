# Puppet agent configuration for HCS machines
class puppet {
  
  if $::machine_type == 'puppetmaster' {
    # More on hiera-eyaml: https://github.com/TomPoulton/hiera-eyaml
    # Ensure that the public and private .pem files are placed in /etc/puppet/secure/keys.
    package { 'hiera-eyaml':
      ensure   => 'installed',
      provider => 'gem',
    }
    
    class { 'puppetdb':
      listen_address     => $::fqdn,
      ssl_listen_address => $::fqdn,
    }
    
    class { 'puppetdb::master::config':
      puppetdb_server => $::fqdn,
      restart_puppet  => false,
    }
    
    # Clean up old reports
    tidy { "/var/lib/puppet/reports":
      age     => "1w",
      recurse => true,
    }
  }

  $puppetmaster_server = hiera('puppetmaster-server')
  
  if $::machine_type == 'bifrost' {
    # Set up cron to query puppet master every 10 minutes.
    cron { 'puppet':
      command => "/usr/bin/puppet agent --onetime --no-daemonize --no-splay --server ${puppetmaster_server} > /dev/null 2>&1",
      user    => 'root',
      minute  => '*/10',
      ensure  => present
    }
  }
  else {
    # Set up cron to query puppet master every hour.
    cron { 'puppet':
      command => "/usr/bin/puppet agent --onetime --no-daemonize --no-splay --server ${puppetmaster_server} > /dev/null 2>&1",
      user    => 'root',
      minute  => fqdn_rand( 60 ), # random minute to load balance queries
      ensure  => present
    }
  }
  
}
