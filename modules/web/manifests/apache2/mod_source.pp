# resource to install an apache2 mod
define web::apache2::mod_source () {
  
  file { "/usr/lib/apache2/modules/mod_${title}.c":
    ensure  => file,
    source  => "puppet:///modules/web/apache2/mods/mod_${title}.c",
    owner   => root,
    group   => root,
    require => Package['apache2'],
  } ~>
  exec { "/usr/bin/apxs -a -i -c /usr/lib/apache2/modules/mod_${title}.c":
    refreshonly => true,
    require     => Package['apache2-dev'],
    notify      => Service['apache2'],
  }
  
}

