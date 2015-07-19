# Request Tracker 4 configuration
class web::request_tracker {

    package { 'request-tracker4': }
    
    package { 'rt4-apache2':
      require => Package['request-tracker4'],
    }
    
    package { 'rt4-clients':
      require => Package['request-tracker4'],
    }
    
    package { 'libnet-ldap-perl':
      require => Package['request-tracker4'],
    }
    
    #file { '/etc/request-tracker4/RT_SiteConfig.d':
    #  ensure  => directory,
    #  recurse => true,
    #  purge   => true,
    #  source  => 'puppet:///modules/web/request-tracker4/RT_SiteConfig.d',
    #  owner   => 'root',
    #  group   => 'root',
    #  mode    => '0600',
    #  require => Package['request-tracker4'],
    #  notify  => Exec['/usr/sbin/update-rt-siteconfig-4'],
    #}
    
    #$rt_database_password = hiera('rt-database-password')
    #file { '/etc/request-tracker4/RT_SiteConfig.d/51-dbconfig':
    #  ensure  => file,
    #  content => template('web/request-tracker4/RT_SiteConfig.d/51-dbconfig.erb'),
    #  owner   => 'root',
    #  group   => 'root',
    #  mode    => '0600',
    #  require => Package['request-tracker4'],
    #  notify  => Exec['/usr/sbin/update-rt-siteconfig-4'],
    #}
    
    #exec { '/usr/sbin/update-rt-siteconfig-4':
    #  refreshonly => true,
    #  notify      => Service['apache2'],
    #}
    
    exec { '/usr/bin/cpan -i RT::Authen::ExternalAuth':
      environment => 'PERL_MM_USE_DEFAULT=1',
      creates     => '/usr/local/share/request-tracker4/plugins/RT-Authen-ExternalAuth',
      require     => [Package['build-essential'], Package['request-tracker4']],
    }
    
}