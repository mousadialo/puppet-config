## HCS users for all servers
class base::users {

  user { 'hcs':
    ensure   => present,
    gid      => 'root',
    home     => '/local/home/hcs',
    comment  => 'HCS root user',
    shell    => '/bin/bash',
    # the SHA1 hash we used didn't really work..
    password => '$6$0uaOzhoY$UpDttMyo/X40L0bhxsQ6v.tQAIiBSs0e4vi8zsPRfu8Ga6wZOoZJL1TfCSxfN5b4r0B8CHla4HaYH/M3AbyV/.',
    #    password => 'e434bed16ace96285481d0d00dc2565894256a74',
    groups   => ['adm', /*'admin-lite',*/ 'admin']
    #TODO remove admin, make admin-lite the gid and add it to to sudoers like on cato
  } ->

  file { '/local':
    ensure => directory
  } ->
  file { '/local/home':
    ensure => directory
  } ->
  file { '/local/home/hcs':
    ensure => directory,
    owner  => 'hcs',
    group  => 'root',
  } ->

  file {
    '/local/home/hcs/.ssh':
      ensure => directory,
      owner  => 'hcs',
      group  => 'root',
      mode   => '0700';

    '/local/home/hcs/.ssh/authorized_keys':
      ensure => present,
      owner  => 'hcs',
      group  => 'root',
      mode   => '0600',
      content => hiera('HCS-public-key')
  }
}
