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
    
    file { '/etc/skel':
      ensure  => directory,
      recurse => true,
      purge   => true,
      source  => 'puppet:///modules/hcsscripts/acctutils/data/skel',
      owner   => 'root',
      group   => 'root',
      # To prevent these files from appearing in the hcs home dir
      require => User['hcs'],
    }
    
    file { '/etc/hcs.bashrc':
      ensure  => file,
      source  => 'puppet:///modules/hcsscripts/acctutils/data/hcs.bashrc',
      owner   => 'root',
      group   => 'root',
    }
    
    file { '/etc/hcs.tcshrc':
      ensure  => file,
      source  => 'puppet:///modules/hcsscripts/acctutils/data/hcs.tcshrc',
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
    
    hcsscripts::acctutils_script { 'hcs-adduser': }
    hcsscripts::acctutils_script { 'hcs-buildmentorshipemails': }
    hcsscripts::acctutils_script { 'hcs-chsh': }
    hcsscripts::acctutils_script { 'hcs-deluser': }
    hcsscripts::acctutils_script { 'hcs-expiry': }
    hcsscripts::acctutils_script { 'hcs-grps': }
    hcsscripts::acctutils_script { 'hcs-k5login2ldap': }
    hcsscripts::acctutils_script { 'hcs-passwords': }
    hcsscripts::acctutils_script { 'hcs-puppetpull': }
    hcsscripts::acctutils_script { 'hcs-resetpassword': }
    hcsscripts::acctutils_script { 'hcs-sqladmin': }
    
  }
}