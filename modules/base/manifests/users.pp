## HCS users for all servers
class base::users {

  file { [ '/local', '/local/home' ]:
    ensure => directory
  } ->
  
  user { 'ubuntu':
    ensure     => absent,
    managehome => true,
  } ->
  
  user { 'hcs':
    ensure         => present,
    uid            => 500
    gid            => 'root',
    home           => '/local/home/hcs',
    managehome     => true,
    comment        => 'HCS Root User',
    shell          => '/bin/bash',
    password       => '$6$0uaOzhoY$UpDttMyo/X40L0bhxsQ6v.tQAIiBSs0e4vi8zsPRfu8Ga6wZOoZJL1TfCSxfN5b4r0B8CHla4HaYH/M3AbyV/.',
    groups         => ['adm', 'sudo'],
    purge_ssh_keys => true,
  } ->
  file { '/local/home/hcs':
    ensure  => directory,
    owner   => 'hcs',
    group   => 'root',
    mode    => '0644',
    recurse => remote,
    source  => 'puppet:///modules/base/home',
  } ->
  ssh_authorized_key { 'HCS':
    user => 'hcs',
    type => 'ssh-rsa',
    key  => hiera('HCS-public-key'),
  }
}
