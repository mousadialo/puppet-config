class sshd {

  file {'/etc/ssh/sshd_config':
    ensure => file,
    source => $::machine_type ? {
      'bifrost' => 'puppet:///modules/sshd/sshd_config.bifrost', # Listen port 2222 instead
      'login'   => 'puppet:///modules/sshd/sshd_config.login', # Allows users other than hcs to log in.
      default   => 'puppet:///modules/sshd/sshd_config',
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
