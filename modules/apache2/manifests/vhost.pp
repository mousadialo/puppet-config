# function to drop apache mod files
define apache2::vhost($template = false) {
  include apache2

  if $template {
    file {"${title}-site-available":
      ensure  => file,
      path    => "/etc/apache2/sites-available/${title}",
      content  => template("apache2/sites/${title}"),
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    }
  }
  else {
    file {"${title}-site-available":
    ensure  => file,
    path    => "/etc/apache2/sites-available/${title}",
    source  => "puppet:///modules/apache2/apache2/sites-available/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package['apache2'],
    }
  }

  file {"${title}-site-enabled":
    ensure  => link,
    path    => "/etc/apache2/sites-enabled/${title}",
    target  => "/etc/apache2/sites-available/${title}",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => [Package['apache2'], File["${title}-site-available"]],
  }

}
