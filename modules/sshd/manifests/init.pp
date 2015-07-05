class sshd {

  service { 'ssh':
    ensure => running,
    enable => true,
  }
  
  file {'/etc/ssh/sshd_config':
    ensure => file,
    source => $::machine_type ? {
      'web'   => 'puppet:///modules/sshd/sshd_config.web', # Web config is a little different in order to make helios work.
      'login' => 'puppet:///modules/sshd/sshd_config.login', # Login config is a little different in order to make Kerberos work.
      default => 'puppet:///modules/sshd/sshd_config',
    },
    owner  => 'root',
    group  => 'root',
    mode   => '0644',
    notify => Service['ssh']
  }

}
