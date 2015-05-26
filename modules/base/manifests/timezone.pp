# Timezone -> America/New_York
class base::timezone {

  file { '/etc/timezone':
    ensure  => present,
    content => "America/New_York\n",
  } ~>
  exec { 'reconfigure-tzdata':
    user    => root,
    group   => root,
    command =>
      '/usr/sbin/dpkg-reconfigure --frontend noninteractive tzdata',
  } ->
  notify { 'timezone-changed':
    message => 'Timezone was updated to America/New_York.',
  }

}
