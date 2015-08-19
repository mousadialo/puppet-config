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

  if $::machine_type == 'login' {
    # Install server banner.
    file { '/etc/banner':
      ensure => file,
      source => 'puppet:///modules/sshd/banner.login',
      owner  => 'root',
      group  => 'root',
    }
    
    # Install SSH host keys.
    file { '/etc/ssh/ssh_host_dsa_key':
      ensure  => file,
      content => hiera('ssh_host_dsa_key'),
      owner   => 'root',
      group   => 'root',
      mode    => '0600',
    }
    
    file { '/etc/ssh/ssh_host_dsa_key.pub':
      ensure => file,
      source => 'puppet:///modules/sshd/ssh_host_dsa_key.pub',
      owner  => 'root',
      group  => 'root',
      mode   => '0644',
    }
    
    file { '/etc/ssh/ssh_host_rsa_key':
      ensure  => file,
      content => hiera('ssh_host_rsa_key'),
      owner   => 'root',
      group   => 'root',
      mode    => '0600',
    }
    
    file { '/etc/ssh/ssh_host_rsa_key.pub':
      ensure => file,
      source => 'puppet:///modules/sshd/ssh_host_rsa_key.pub',
      owner  => 'root',
      group  => 'root',
      mode   => '0644',
    }
    
    file { '/etc/ssh/ssh_host_ecdsa_key':
      ensure  => file,
      content => hiera('ssh_host_ecdsa_key'),
      owner   => 'root',
      group   => 'root',
      mode    => '0600',
    }
    
    file { '/etc/ssh/ssh_host_ecdsa_key.pub':
      ensure => file,
      source => 'puppet:///modules/sshd/ssh_host_ecdsa_key.pub',
      owner  => 'root',
      group  => 'root',
      mode   => '0644',
    }
  }

}
