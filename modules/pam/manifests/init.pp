# HCS PAM configurations
class pam {

  file { '/etc/pam.d/sshd':
    ensure => file,
    source => 'puppet:///modules/pam/sshd',
    owner  => 'root',
    group  => 'root'
  }
  
  if $::machine_type == 'login' {
    # Removed use_authtok to allow users to change password using passwd
    file { '/etc/pam.d/common-password':
      ensure => file,
      source => 'puppet:///modules/pam/common-password',
      owner  => 'root',
      group  => 'root'
    }
  }

}
