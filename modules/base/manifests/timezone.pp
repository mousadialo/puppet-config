# Timezone -> America/New_York
class base::timezone {

  file { '/etc/timezone':
    ensure  => file,
    content => "America/New_York\n",
  } ~>
  exec { 'reconfigure-tzdata':
    command     => '/usr/sbin/dpkg-reconfigure --frontend noninteractive tzdata',
    user        => root,
    group       => root,
    refreshonly => true
  } ->
  notify { 'timezone-changed':
    message => 'Timezone was updated to America/New_York.',
  }

}
