define web::modsecurity::rule ($rule_type) {

  validate_re($rule_type, '^(experimental|optional|slr)$',
    "${rule_type} is not supported for rule_type.
    Allowed values are 'experimental', 'optional', and 'slr'.")

  file { "/usr/share/modsecurity-crs/activated_rules/${title}":
    ensure  => link,
    target  => "/usr/share/modsecurity-crs/${rule_type}_rules/${title}",
    owner   => 'root',
    group   => 'root',
    require => Package['modsecurity-crs'],
  }

}