# Installs and enables php5 modules.
define apache2::php5_mod() {
  include apache2
  
  package { "php5-${title}":
    ensure => installed,
  }
  
  file { ["/etc/php5/apache2/conf.d/20-${title}.ini",
          "/etc/php5/cgi/conf.d/20-${title}.ini",
          "/etc/php5/cli/conf.d/20-${title}.ini"]:
    ensure  => link,
    target  => "../../mods-available/${title}.ini",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package["php5-${title}"],
  }
  
}

