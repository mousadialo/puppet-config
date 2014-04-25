# HCS PAM configurations
class pam {

  file { '/etc/pam.d/sshd':
    ensure => present,
    source => 'puppet:///modules/pam/sshd',
    owner  => 'root',
    group  => 'root',
    notify => Notify['pam-sshd-changed']
  }

  notify { 'pam-sshd-changed':
    message => 'PAM sshd was updated.
    You need to log in again before changes apply'
  }

}
