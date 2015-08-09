# HCS filesystem configuration
class filesystem {

  require base
  require hosts

  $nfs_server = hiera('nfs-server')
  $zpool_name = hiera('zfs::zpool_name')
  $mount_dir = hiera('nfs-mount-dir')

  if $::machine_type != 'file' {
    include nfs::client
  }
  
  if $::machine_type == 'web' {
    # Mount our web directories
    nfs::client::mount { 'www-hcs.harvard.edu':
      server  => $nfs_server,
      share   => "/${zpool_name}/services/www-hcs.harvard.edu",
      mount   => "${mount_dir}/www-hcs.harvard.edu",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'webapps',
      group   => 'hcs',
    }

    nfs::client::mount { 'www-hcs.harvard.edu-ssl':
      server  => $nfs_server,
      share   => "/${zpool_name}/services/www-hcs.harvard.edu-ssl",
      mount   => "${mount_dir}/www-hcs.harvard.edu-ssl",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'webapps',
      group   => 'hcs',
    }
    
    # Mount PHP sessions directory
    nfs::client::mount { 'sessions':
      server  => $nfs_server,
      share   => "/${zpool_name}/services/sessions",
      mount   => "${mount_dir}/sessions",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'root',
      group   => 'root',
      perm    => '1733',
    }
  }
  
  if $::machine_type == 'web' or $::machine_type == 'login' {
    # Mount sshkeys directory for helios or ssh login
    nfs::client::mount { 'sshkeys':
      server  => $nfs_server,
      share   => "/${zpool_name}/services/sshkeys",
      mount   => "${mount_dir}/sshkeys",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'webapps',
      group   => 'hcs',
      perm    => '0711',
    }
  }
  
  if $::machine_type == 'lists' {
    # Mount mailman directory
    nfs::client::mount { 'mailman':
      server  => $nfs_server,
      share   => "/${zpool_name}/services/mailman",
      mount   => "${mount_dir}/mailman",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'list',
      group   => 'list',
    }
  }
  
  if $::machine_type == 'lists' or $::machine_type == 'mail' {
    # Mount transport directory
    nfs::client::mount { 'transport':
      server  => $nfs_server,
      share   => "/${zpool_name}/services/transport",
      mount   => "${mount_dir}/transport",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'list',
      group   => 'list',
    }
  }

  if $::machine_type == 'web' or $::machine_type == 'mail' or $::machine_type == 'login' {
    nfs::client::mount { 'home':
      server  => $nfs_server,
      share   => "/${zpool_name}/home",
      mount   => "${mount_dir}/home",
      options => 'nfsvers=3,rw,relatime,nosuid,nodev',
      atboot  => true,
      owner   => 'root',
      group   => 'root',
    }
  
    # We want to symlink our home directory to nfs
    file { 'home':
      ensure  => link,
      path    => '/home',
      target  => "${mount_dir}/home",
      force   => true,
      owner   => 'root',
      group   => 'root',
      # Requirements:
      # 1) Must have mounted nfs
      # 2) Ubuntu user is deleted or has a new local home directory
      require => [Nfs::Client::Mount['home'], User['ubuntu']],
    }

    # HACK nscd caches all of the users groups from ldap. Whenever a sudo
    # user does an apt-get their groups get all screwed up. We restart nscd
    # everytime we do an apt-get so their groups are synced to ldap. This
    # file is going here because if we symlink the home directory it means
    # that ldap users are going to be logging in.
    #file { '/etc/apt/apt.conf.d/00restartnscd':
    #  ensure => file,
    #  source => 'puppet:///modules/filesystem/nscd-restart',
    #  owner  => 'root',
    #  group  => 'root',
    #}
    
    package { 'autofs':
      ensure  => installed,
      require => Nfs::Client::Mount['home'],
    }
    
    service { 'autofs':
      ensure  => running,
      enable  => true,
      require => Package['autofs'],
    }

    file { '/etc/auto.master':
      ensure  => file,
      source  => 'puppet:///modules/filesystem/autofs/auto.master',
      owner   => 'root',
      group   => 'root',
      notify  => Service['autofs'],
      require => Package['autofs'],
    }

    file { '/etc/autofs':
      ensure => directory,
      owner  => 'root',
      group  => 'root',
    }

    file { '/etc/autofs/nfs.people':
      ensure  => file,
      content => template('filesystem/autofs/nfs.people'),
      owner   => 'root',
      group   => 'root',
      notify  => Service['autofs'],
      require => [Package['autofs'], File['/etc/autofs']],
    }

    file { '/etc/autofs/nfs.groups':
      ensure  => file,
      content => template('filesystem/autofs/nfs.groups'),
      owner   => 'root',
      group   => 'root',
      notify  => Service['autofs'],
      require => [Package['autofs'], File['/etc/autofs']],
    }

    file { '/etc/autofs/nfs.general':
      ensure  => file,
      content => template('filesystem/autofs/nfs.general'),
      owner   => 'root',
      group   => 'root',
      notify  => Service['autofs'],
      require => [Package['autofs'], File['/etc/autofs']],
    }

    file { '/etc/autofs/nfs.hcs':
      ensure  => file,
      content => template('filesystem/autofs/nfs.general'),
      owner   => 'root',
      group   => 'root',
      notify  => Service['autofs'],
      require => [Package['autofs'], File['/etc/autofs']],
    }
  }
  
  if defined(File['/etc/nsswitch.conf']) {
    # If nsswitch.conf is defined, then this machine is also an LDAP client.
    
    service { 'idmapd':
      ensure => running,
      enable => true,
    }
    
    # This file is used for mapping user ids and group ids between filer and
    # clients. It should be identical on clients and server
    file { '/etc/idmapd.conf':
      ensure  => file,
      source  => 'puppet:///modules/filesystem/idmapd.conf',
      owner   => 'root',
      group   => 'root',
      mode    => '0644',
      require => File['/etc/nsswitch.conf'],
      notify  => Service['idmapd'],
    }
  }

}
