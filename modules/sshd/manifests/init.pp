class sshd {

  file {'/etc/ssh/sshd_config':
    ensure => file,
    source => $::machine_type ? {
      'login' => 'puppet:///modules/sshd/sshd_config.login', # Allows users other than hcs to log in.
      default => 'puppet:///modules/sshd/sshd_config',
    },
    owner  => 'root',
    group  => 'root',
    mode   => '0644',
  } ~>
  service { 'ssh':
    ensure => running,
    enable => true,
  }

}
