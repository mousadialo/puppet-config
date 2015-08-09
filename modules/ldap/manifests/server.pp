# HCS LDAP server configuration
# Look at the README for instructions on setting up an LDAP server
class ldap::server {
  
  $hashed_root_dn_pwd = hiera('hashed_root_dn_pwd')
  $root_dn_pwd = hiera('root_dn_pwd')

  package { '389-ds-base':
    ensure => installed,
  }
  
  package { 'libnss3-tools':
    ensure => installed,
  }
  
  file { '/etc/dirsrv/schema/00core.ldif':
    ensure  => file,
    source  => 'puppet:///modules/ldap/00core.ldif',
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
    require => Package['389-ds-base'],
  }
  
  file { '/etc/dirsrv/config/setup.inf':
    ensure  => file,
    content => template('ldap/setup.inf.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0400',
    require => Package['389-ds-base'],
  }
  
  file { '/etc/dirsrv/config/setupssl.sh':
    ensure  => file,
    source  => 'puppet:///modules/ldap/setupssl.sh',
    owner   => 'root',
    group   => 'root',
    mode    => '0700',
    require => Package['389-ds-base'],
  }
  
  exec { 'setup-ds':
    command => '/usr/sbin/setup-ds --silent --file=/etc/dirsrv/config/setup.inf',
    creates => "/etc/dirsrv/slapd-${::hostname}",
    user    => 'root',
    require => File['/etc/dirsrv/config/setup.inf'],
    notify  => [Exec['setup-ds-ssl'], Exec['import-backup']],
  }
  
  exec { 'setup-ds-ssl':
    command     => "/etc/dirsrv/config/setupssl.sh /etc/dirsrv/slapd-${::hostname}",
    environment => "DMPWD=${root_dn_pwd}",
    refreshonly => true,
    user        => 'root',
    require     => [File['/etc/dirsrv/config/setupssl.sh'], Package['libnss3-tools'], Service['dirsrv']],
  }
  
  exec { 'import-backup':
    #                       list all files in ldap folder             get latest backup         extract filename                                 retrieve file from S3                            decompress file                                       import to LDAP
    command     => "/usr/local/bin/aws s3 ls s3://hcs-backups/ldap/ | /usr/bin/tail -n 1 | /usr/bin/awk \'{print \$4}\' | /usr/bin/xargs -I % /usr/local/bin/aws s3 cp s3://hcs-backups/ldap/% - | /bin/gunzip -c | /usr/bin/ldapadd -xc -D \"cn=Directory Manager\" -w \"${root_dn_pwd}\" -H ldap://localhost",
    refreshonly => true,
    user        => 'root',
    require     => [Class['awscli'], Service['dirsrv']],
  }
  
  service { 'dirsrv':
    ensure  => running,
    enable  => true,
    require => Exec['setup-ds'],
  }
  
  file { '/etc/cron.daily/backup-ldap':
    ensure => file,
    content => template('ldap/backup-ldap.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0755',
    require => Class['awscli'],
  }
  
  @@haproxy::balancermember { "${::hostname}-ldap":
    listening_service => 'ldap',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['389'],
    options           => ['check'],
  }
  
  @@haproxy::balancermember { "${::hostname}-ldaps":
    listening_service => 'ldaps',
    server_names      => $::fqdn,
    ipaddresses       => $::ipaddress,
    ports             => ['636'],
    options           => ['check'],
  }
  
}
