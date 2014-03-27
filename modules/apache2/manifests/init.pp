class apache2 {

  package { 'apache2':
    ensure => installed
  }

  service { 'apache2':
    ensure => true,
    enable => true,
    require => Package['apache2']
  }

  # drop all the apache2 files
  file { '/etc/apache2':
    ensure  => directory,
    recurse => true,
    source  => 'puppet:///modules/apache2'
  }
  /*###########################################
    List of apache2 files required:
    '/etc/apache2/hcs-conf.d/tilde-rewrites.conf'
    '/etc/apache2/hcs-conf.d/redirects.conf'
    '/etc/apache2/hcs-conf.d/userdir.conf'
    '/etc/apache2/hcs-conf.d/directories.conf'

  *###########################################*/
}
