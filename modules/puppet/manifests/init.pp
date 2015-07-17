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
      listen_address     => 'puppetmaster.hcs.so',
      ssl_listen_address => 'puppetmaster.hcs.so',
    }
    
    class { 'puppetdb::master::config':
      puppetdb_server => 'puppetmaster.hcs.so',
      restart_puppet  => false,
    }
  }
  
  if $::machine_type == 'bifrost' {
    # Set up cron to query puppet master every 10 minutes.
    cron { 'puppet':
      command => '/usr/bin/puppet agent --onetime --no-daemonize --no-splay --server puppetmaster.hcs.so > /dev/null 2>&1',
      user    => 'root',
      minute  => '*/10',
      ensure  => present
    }
  }
  else {
    # Set up cron to query puppet master every hour.
    cron { 'puppet':
      command => '/usr/bin/puppet agent --onetime --no-daemonize --no-splay --server puppetmaster.hcs.so > /dev/null 2>&1',
      user    => 'root',
      minute  => fqdn_rand( 60 ), # random minute to load balance queries
      ensure  => present
    }
  }
  
}
