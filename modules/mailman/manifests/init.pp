# mailman configuration
class mailman {

  require nfs
  require web
  require mail
  
  $domain = hiera('domain')
  $mount_dir = hiera('nfs-mount-dir')
  
  # This was tested with mailman version 1:2.1.16-2ubuntu0.1. When upgrading
  # mailman, test out the patches on a generic server first before making the
  # actual upgrade.
  package { 'mailman':
    ensure => '1:2.1.16-2ubuntu0.1',
  } ~>
  exec { '/usr/bin/patch -p1 < mailman-hcs.diff':
    cwd         => '/usr/lib/mailman',
    refreshonly => true,
    require     => File['/usr/lib/mailman/mailman-hcs.diff'],
    notify      => Service['mailman'],
  }
  
  service { 'mailman':
    ensure  => running,
    enable  => true,
    require => Package['mailman'],
  }
  
  file { '/usr/lib/mailman/mailman-hcs.diff':
    ensure  => file,
    source  => 'puppet:///modules/mailman/mailman-hcs.diff',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['mailman'],
  }
  
  file { '/etc/mailman/en':
    ensure  => directory,
    recurse => remote,
    source  => 'puppet:///modules/mailman/templates/en',
    owner   => 'root',
    group   => 'root',
    require => Package['mailman'],
  }
  
  file { '/etc/mailman/mm_cfg.py':
    ensure  => file,
    content => template('mailman/mm_cfg.py.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['mailman'],
    notify  => Service['mailman'],
  }

  # Symlink mailman files to appropriate location
  file { '/var/lib/mailman/archives':
    ensure  => link,
    target  => '${mount_dir}/mailman/archives',
    force   => true,
    owner   => 'root',
    group   => 'list',
    require => [Nfs::Client::Mount['mailman'], Package['mailman']],
    notify  => Service['mailman'],
  }

  file { '/var/lib/mailman/data':
    ensure  => link,
    target  => '${mount_dir}/mailman/data',
    force   => true,
    owner   => 'root',
    group   => 'list',
    require => [Nfs::Client::Mount['mailman'], Package['mailman']],
    notify  => Service['mailman'],
  }
  
  file { '/var/lib/mailman/lists':
    ensure  => link,
    target  => '${mount_dir}/mailman/lists',
    force   => true,
    owner   => 'root',
    group   => 'list',
    require => [Nfs::Client::Mount['mailman'], Package['mailman']],
    notify  => Service['mailman'],
  }
  
}