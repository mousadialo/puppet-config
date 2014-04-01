# function to drop apache mod files
define apache2::vhost() {
  include apache2

  file {"${title}-site-availible":
    ensure  => file,
    path    => "/etc/apache2/sites-availible/${title}",
    source  => "puppet:///modules/apache2/apache2/sites-availible/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package['apache2'],
  }

  file {"${title}-site-enabled":
    ensure  => link,
    path    => "/etc/apache2/sites-enabled/${title}",
    target  => "/etc/apache2/sites-availible/${title}",
    owner   => 'root',
    group   => 'root',
    require => [Package['apache2'], File["${title}-site-availible"]],
  }

}
