# With conf specifies whether conf file should be linked as well.
define apache2::mod($ensure = 'enabled', $with_conf = false) {
  include apache2
  
  validate_re($ensure, '^(enabled|disabled)$',
    "${ensure} is not supported for ensure.
    Allowed values are 'enabled' and 'disabled'.")
  
  if $ensure == 'enabled' {
    file { "${title}-mod-load":
      ensure  => link,
      path    => "/etc/apache2/mods-enabled/${title}.load",
      target  => "../mods-available/${title}.load",
      owner   => 'root',
      group   => 'root',
      notify  => Service['apache2'],
      require => Package['apache2'],
    }

    if $with_conf {
      file { "${title}-mod-conf":
        ensure  => link,
        path    => "/etc/apache2/mods-enabled/${title}.conf",
        target  => "../mods-available/${title}.conf",
        owner   => 'root',
        group   => 'root',
        notify  => Service['apache2'],
        require => Package['apache2'],
      }
    }
  }
  else {
    file { "${title}-mod-load":
      ensure  => absent,
      path    => "/etc/apache2/mods-enabled/${title}.load",
      notify  => Service['apache2'],
      require => Package['apache2'],
    }

    if $with_conf {
      file { "${title}-mod-conf":
        ensure  => absent,
        path    => "/etc/apache2/mods-enabled/${title}.conf",
        notify  => Service['apache2'],
        require => Package['apache2'],
      }
    }
  }
}

