# function to drop generic apache2 configuration files
define apache2::config_file () {
  include apache2

  file {$title:
    ensure  => file,
    path    => "/etc/apache2/${title}",
    source  => "puppet:///modules/apache2/apache2/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package['apache2'],
  }

  Class['apache2'] -> Apache2::Config_file["$title"]
}
