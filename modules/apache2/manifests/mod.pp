/* With conf specifies whether conf file should be copied as well. */
define apache2::mod($with_conf = false) {
  include apache2

  file {"${title}-mod-availible":
    ensure  => file,
    path    => "/etc/apache2/mods-availible/{$title}.load",
    source  => "puppet:///modules/apache2/apache2/mods-availible/{$title}.load",
    owner   => root
    group   => root
    notify  => Service['apache2'],
    require => Package['apache2'],
  }

  file {"${title}-mod-enabled":
    path    => "/etc/apache2/mods-enabled/{$title}.load",
    ensure  => link,
    target  => "/etc/apache2/mods-availible/{$title}.load",
    owner   => root
    group   => root
    notify  => Service['apache2'],
    require => Package['apache2'],
    require => File["${title}-mod-availible"],
  }

  if $with_conf {
    file {"${title}-mod-availible-conf":
      ensure  => file,
      path    => "/etc/apache2/mods-availible/{$title}.conf",
      source  => "puppet:///modules/apache2/apache2/mods-availible/{$title}.conf",
      owner   => root
      group   => root
      require => Package['apache2'],
    }

    file {"${title}-mod-enabled-conf":
      path    => "/etc/apache2/mods-enabled/{$title}.conf",
      ensure  => link,
      target  => "/etc/apache2/mods-availible/{$title}.conf",
      owner   => root
      group   => root
      require => Package['apache2'],
      require => File["${title}-mod-availible"],
    }
  }
}
