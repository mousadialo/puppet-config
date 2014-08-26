# function to drop generic phpyadmin configuration files
define phpmyadmin::config_file () {
  include phpmyadmin

  file {$title:
    ensure  => file,
    path    => "/etc/phpmyadmin/${title}",
    source  => "puppet:///modules/phpmyadmin/phpmyadmin/${title}",
    owner   => 'root',
    group   => 'root',
    require => Package['phpmyadmin'],
  }

}
