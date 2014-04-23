class base::users {
  file { '/local':
    ensure => directory
  } ->
  file { '/local/home':
    ensure => directory
  } ->
  file { '/local/home/ubuntu':
    ensure => directory,
    source => '/home/ubuntu',
    recurse => true
  } ->
  user { 'ubuntu':
    ensure => present,
    home => '/local/home/ubuntu'
  }
}
