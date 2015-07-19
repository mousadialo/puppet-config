# Shibboleth configuration
class web::shibboleth {

    package { 'libapache2-mod-shib2':
      ensure  => installed,
      require => Package['apache2'],
    }
    
    web::apache2::mod { 'shib2':
      require => Package['libapache2-mod-shib2'],
    }
    
    # Main shibboleth configuration file
    file { '/etc/shibboleth/shibboleth2.xml':
      ensure  => file,
      source  => 'puppet:///modules/web/shibboleth/shibboleth2.xml',
      owner   => 'root',
      group   => 'root',
      require => Package['libapache2-mod-shib2'],
      notify  => Service['shibd'],
    }
    
    # HUIT IDP metadata
    file { '/etc/shibboleth/huit-idp-metadata.xml':
      ensure  => file,
      source  => 'puppet:///modules/web/shibboleth/huit-idp-metadata.xml',
      owner   => 'root',
      group   => 'root',
      require => Package['libapache2-mod-shib2'],
      notify  => Service['shibd'],
    }
    
    # Attribute Map for HUIT IDP
    file { '/etc/shibboleth/attribute-map.xml':
      ensure  => file,
      source  => 'puppet:///modules/web/shibboleth/attribute-map.xml',
      owner   => 'root',
      group   => 'root',
      require => Package['libapache2-mod-shib2'],
      notify  => Service['shibd'],
    }
    
    service { 'shibd':
      ensure  => running,
      enable  => true,
      require => Package['libapache2-mod-shib2'],
    }
    
}