# phpmyadmin configuration

class phpmyadmin {
  
  package { 'phpmyadmin':
    ensure => installed 
  }

  if $::machine_type == 'web' {
    phpmyadmin::config_file { 'apache.conf': }
    phpmyadmin::config_file { 'config.inc.php': }
  }
}
