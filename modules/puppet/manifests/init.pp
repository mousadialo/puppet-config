# Puppet agent configuration for HCS machines
class puppet {

  # ensure puppetmaster IP is in the /etc/hosts file
  host { 'puppetmaster.hcs.so':
    ensure       => present,
    host_aliases => 'puppet',
    ip           => hiera('puppetmaster-server'),
    comment      => 'Puppetmaster IP Address'
  }
  
  # set up cron to query puppet master every hour
  cron { 'puppet':
    command => '/usr/bin/puppet agent --onetime --no-daemonize --no-splay > /dev/null 2>&1',
    user    => 'root',
    minute  => fqdn_rand( 60 ), # random minute to load balance queries
    ensure  => present
  }
  
}