# Memcached configuration
class web::memcached {

  package { 'memcached': } ->
  service { 'memcached':
    ensure => running,
    enable => true,
  }
  
  file_line { 'memcached_memory_cap':
    path  => '/etc/memcached.conf',
    line  => '-m 1024',
    match => '^-m \d+$',
    require => Package['memcached'],
    notify  => Service['memcached'],
  }
  
  file_line { 'memcached_listen_address':
    path    => '/etc/memcached.conf',
    line    => "-l 127.0.0.1,${::ipaddress}",
    match   => '^-l .+',
    require => Package['memcached'],
    notify  => Service['memcached'],
  }

}