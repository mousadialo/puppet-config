## HCS users for all servers
class base::users {

  user { 'hcs':
    ensure   => present,
    gid      => 'root',
    home     => '/local/home/hcs',
    comment  => 'HCS root user',
    shell    => '/bin/bash',
    password => 'e434bed16ace96285481d0d00dc2565894256a74',
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
