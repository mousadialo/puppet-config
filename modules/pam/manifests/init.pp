# HCS PAM configurations
class pam {

  file { '/etc/pam.d/sshd':
    ensure => file,
    source => 'puppet:///modules/pam/sshd',
    owner  => 'root',
    group  => 'root'
  }

}
