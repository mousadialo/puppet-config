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
  
  web::modsecurity::rule { 'modsecurity_crs_20_protocol_violations.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_23_request_limits.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_30_http_policy.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_35_bad_robots.conf':
    type => 'base',
  }
  web::modsecurity::rule { 'modsecurity_35_bad_robots.data':
    type => 'base',
  }
  web::modsecurity::rule { 'modsecurity_35_scanners.data':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_40_generic_attacks.conf':
    type => 'base',
  }
  web::modsecurity::rule { 'modsecurity_40_generic_attacks.data':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_41_sql_injection_attacks.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_41_xss_attacks.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_42_tight_security.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_45_trojans.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_47_common_exceptions.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_49_inbound_blocking.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_60_correlation.conf':
    type => 'base',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_42_comment_spam.conf':
    type => 'optional',
  }
  web::modsecurity::rule { 'modsecurity_42_comment_spam.data':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_47_skip_outbound_checks.conf':
    type => 'optional',
  }
  
  web::modsecurity::rule { 'modsecurity_crs_46_slr_et_wordpress_attacks.conf':
    type => 'slr',
  }
  web::modsecurity::rule { 'modsecurity_46_slr_et_wordpress.data':
    type => 'slr',
  }
  
}