class sshd {

  service { 'ssh':
    ensure    => running,
    enable    => true,
  }

  if $::machine_type == 'web' {
    # Web config is a little different in order to make helios work
    file {'/etc/ssh/sshd_config':
      ensure => file,
      source => 'puppet:///modules/ssh/sshd_config.web',
      owner  => 'root',
      group  => 'root',
      mode   => '0644',
      notify => Service['ssh']
    }
  } else {
    file {'/etc/ssh/sshd_config':
      ensure => file,
      source => 'puppet:///modules/ssh/sshd_config',
      owner  => 'root',
      group  => 'root',
      mode   => '0644',
      notify => Service['ssh']
    }
  }
}
