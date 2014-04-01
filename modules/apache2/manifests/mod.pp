# With conf specifies whether conf file should be copied as well.
define apache2::mod($with_conf = false) {
  include apache2

  file {"${title}-mod-available":
    ensure  => file,
    path    => "/etc/apache2/mods-available/${title}.load",
    source  => "puppet:///modules/apache2/apache2/mods-available/${title}.load",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => Package['apache2'],
  }

  file {"${title}-mod-enabled":
    ensure  => link,
    path    => "/etc/apache2/mods-enabled/${title}.load",
    target  => "/etc/apache2/mods-available/${title}.load",
    owner   => 'root',
    group   => 'root',
    notify  => Service['apache2'],
    require => [Package['apache2'], File["${title}-mod-available"]], }

  if $with_conf {
    file {"${title}-mod-available-conf":
      ensure  => file,
      path    => "/etc/apache2/mods-available/${title}.conf",
      source  => "puppet:///modules/apache2/apache2/mods-available/${title}.conf",
      owner   => 'root',
      group   => 'root',
      require => Package['apache2'],
    }

    file {"${title}-mod-enabled-conf":
      ensure  => link,
      path    => "/etc/apache2/mods-enabled/${title}.conf",
      target  => "/etc/apache2/mods-available/${title}.conf",
      owner   => 'root',
      group   => 'root',
      require => [Package['apache2'], File["${title}-mod-available-conf"]],
    }
  }

}

