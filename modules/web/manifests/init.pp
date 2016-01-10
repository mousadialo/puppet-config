# web configuration
# will install and configure apache2 and other software required to run web servers
class web {
  
  include web::apache2

  if $::machine_type == 'web' {
    
    # suPHP
    include web::suphp

    # Shibboleth
    include web::shibboleth
    
    # Memcached
    include web::memcached
    
    # ModSecurity
    # TODO(fred): Too many false positives, disabling for now.
    #include web::modsecurity

    # RT
    include web::request_tracker
    
    # safesendmail rate limits PHP mail
    include web::safesendmail
    
    # Python packages
    package { 'python-crypto': }
    package { 'python-django': }
    package { 'python-ecdsa': }
    package { 'python-elixir': }
    package { 'python-flask': }
    package { 'python-gd': }
    package { 'python-imaging': }
    package { 'python-ldap': }
    package { 'python-mysqldb': }
    package { 'python-tz': }
    package { 'python-yaml': }
    
    require filesystem
    $mount_dir = hiera('nfs-mount-dir')

    # Symlink our web files to appropriate location
    file { '/var/www/hcs.harvard.edu':
      ensure  => link,
      target  => "${mount_dir}/www-hcs.harvard.edu",
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Must have mounted www-hcs.harvard.edu
      require => [Nfs::Client::Mount['www-hcs.harvard.edu'], Package['apache2']],
    }

    file { '/var/www/hcs.harvard.edu-ssl':
      ensure  => link,
      target  => "${mount_dir}/www-hcs.harvard.edu-ssl",
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Must have mounted www-hcs.harvard.edu-ssl
      require => [Nfs::Client::Mount['www-hcs.harvard.edu-ssl'], Package['apache2']],
    }
    
    @@haproxy::balancermember { "${::hostname}-web-http":
      listening_service => 'web-http',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['80'],
      define_cookies    => true,
      options           => ['send-proxy', 'check'],
    }
    
    @@haproxy::balancermember { "${::hostname}-web-https":
      listening_service => 'web-https',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['443'],
      define_cookies    => true,
      options           => ['send-proxy', 'check', 'ssl verify none'],
    }
    
    # Python script to reload user vhosts
    file { '/etc/cron.hourly/reload-vhosts':
      ensure  => file,
      source  => 'puppet:///modules/web/cron.hourly/reload-vhosts',
      owner   => 'root',
      group   => 'root',
      mode    => '0755',
      require => Nfs::Client::Mount['vhosts'],
    }
    
    ## Compatibility stuff ##
    
    # Some groups use /nfs/home instead of /home. Point to new NFS mount location.
    file { '/nfs':
      ensure => link,
      target => hiera('nfs-mount-dir'),
      owner  => 'root',
      group  => 'root',
    }
    
    # Old perl location
    file { '/usr/local/bin/perl':
      ensure => link,
      target => '/usr/bin/perl',
      owner  => 'root',
      group  => 'root',
    }
  }
  elsif $::machine_type == 'lists' {
    @@haproxy::balancermember { "${::hostname}-lists-http":
      listening_service => 'lists-http',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['80'],
      define_cookies    => true,
      options           => ['send-proxy', 'check'],
    }
    
    @@haproxy::balancermember { "${::hostname}-lists-https":
      listening_service => 'lists-https',
      server_names      => $::fqdn,
      ipaddresses       => $::ipaddress,
      ports             => ['443'],
      define_cookies    => true,
      options           => ['send-proxy', 'check', 'ssl verify none'],
    }
  }
  
}
