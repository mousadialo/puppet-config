class sshd {

  file { '/etc/ssh/sshd_config':
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

  # Install SSH host keys.
  if $::machine_type == 'login' {
    define host_key () {
      file { "/etc/ssh/ssh_host_${title}_key":
        ensure => file,
        content => hiera("ssh_host_${title}_key"),
        owner  => 'root',
        group  => 'root',
        mode   => '0600',
      }
    
      file { "/etc/ssh/ssh_host_${title}_key.pub":
        ensure => file,
        source => "puppet:///modules/sshd/ssh_host_${title}_key.pub",
        owner  => 'root',
        group  => 'root',
        mode   => '0644',
      }
    }
    
    host_key { 'dsa': }
    host_key { 'rsa': }
    host_key { 'ecdsa': }
  }

}
