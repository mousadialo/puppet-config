class base::users {
  file { '/local':
    ensure => directory
  } ->
  file { '/local/home':
    ensure => directory
  } ->
  file { '/local/home/hcs':
    ensure => directory,
    source => '/home/hcs',
    recurse => true
  } ->
  user { 'hcs':
    ensure => present,
    home => '/local/home/hcs',
    comment => 'HCS root user',
    groups => ['adm', 'admin-lite', 'admin'] #TODO remove admin and add admin-lite to sudoers like on cato
  }
}
