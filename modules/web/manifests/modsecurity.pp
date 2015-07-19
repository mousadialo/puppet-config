# ModSecurity configuration
class web::modsecurity {
  
  package { 'libapache2-modsecurity':
    ensure  => installed,
    require => Package['apache2'],
  }
  
  package { 'modsecurity-crs':
    ensure  => installed,
    require => Package['libapache2-modsecurity'],
  }
    
  web::apache2::config { 'mods-available/security2.conf':
    require => [Package['libapache2-modsecurity'], Package['modsecurity-crs']],
  }
  web::apache2::mod { 'security2':
    require => Package['libapache2-modsecurity'],
  }
  
  file { '/etc/modsecurity':
    ensure  => directory,
    recurse => remote,
    source  => 'puppet:///modules/web/modsecurity',
    owner   => 'root',
    group   => 'root',
    require => Package['libapache2-modsecurity'],
    notify  => Service['apache2'],
  }
  
  web::modsecurity::rule { 'modsecurity_crs_10_ignore_static.conf':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_13_xml_enabler.conf':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_16_session_hijacking.conf':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_42_comment_spam.conf':
    type => 'optional',
  }
  web::modsecurity::rule { 'modsecurity_42_comment_spam.data':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_43_csrf_protection.conf':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_46_slr_et_wordpress_attacks.conf':
    type => 'slr',
  }
  web::modsecurity::rule { 'modsecurity_46_slr_et_wordpress.data':
    type => 'slr',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_47_skip_outbound_checks.conf':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_55_application_defects.conf':
    type => 'optional',
  }
  
}