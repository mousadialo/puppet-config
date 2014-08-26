# phpmyadmin configuration

class phpmyadmin {
  
  package { 'phpmyadmin':
    ensure => installed 
  }

  if $::machine_type == 'web' {

    file{ '/etc/phpmyadmin':
      ensure  => directory,
      require => Package['phpmyadmin'],
    }

    phpmyadmin::config_file { 'apache.conf': }
    phpmyadmin::config_file { 'config.inc.php': }
  }
}
