# function to drop generic apache2 configuration files
define web::apache2_config () {

  file { $title:
    ensure  => file,
    path    => "/etc/apache2/${title}",
    source  => "puppet:///modules/web/apache2/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package['apache2'],
  }

}
