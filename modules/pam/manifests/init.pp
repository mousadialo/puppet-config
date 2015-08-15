# HCS PAM configurations
class pam {

  file { '/etc/pam.d/sshd':
    ensure => file,
    source => 'puppet:///modules/pam/sshd',
    owner  => 'root',
    group  => 'root'
  }
  
  if $::machine_type == 'login' {
    # Has use_authtok removed to allow LDAP users to change passwords.
    file { '/etc/pam.d/common-password':
      ensure => file,
      source => 'puppet:///modules/pam/common-password',
      owner  => 'root',
      group  => 'root'
    }
  }

}
