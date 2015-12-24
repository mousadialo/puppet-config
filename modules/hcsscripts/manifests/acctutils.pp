# Configures acctutils scripts.
class hcsscripts::acctutils {

  if $::machine_type == 'login' {
    
    file { '/usr/lib/python2.7/hcs':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/hcsscripts/acctutils/lib/hcs',
      owner   => 'root',
      group   => 'root',
    }
    
    file { '/usr/lib/python2.7/acctutils':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/hcsscripts/acctutils/lib/acctutils',
      owner   => 'root',
      group   => 'root',
    }
    
    file { '/etc/hcs/mail_templates':
      ensure  => directory,
      recurse => remote,
      source  => 'puppet:///modules/hcsscripts/acctutils/data/mail_templates',
      owner   => 'root',
      group   => 'root',
      require => File['/etc/hcs'],
    }
    
    file { '/etc/hcs/info.yml':
      ensure  => file,
      source  => 'puppet:///modules/hcsscripts/acctutils/data/info.yml',
      owner   => 'root',
      group   => 'root',
      require => File['/etc/hcs'],
    }
    
    define script($file_name = $title) {
      file { "/usr/bin/$file_name":
        ensure  => file,
        source  => "puppet:///modules/hcsscripts/acctutils/bin/$file_name",
        owner   => 'root',
        group   => 'root',
        mode    => '0755',
      }
    }
    script { 'hcs-adduser': }
    script { 'hcs-buildmentorshipemails': }
    script { 'hcs-chsh': }
    script { 'hcs-deluser': }
    script { 'hcs-expiry': }
    script { 'hcs-grps': }
    script { 'hcs-k5login2ldap': }
    script { 'hcs-passwords': }
    script { 'hcs-puppetpull': }
    script { 'hcs-resetpassword': }
    script { 'hcs-sqladmin': }
    
  }
}