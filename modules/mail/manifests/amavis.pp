# AMaViS configuration
class mail::amavis {

  package { ['amavisd-new',
             'spamassassin',
             'libnet-dns-perl',
             'libmail-spf-perl']:
    ensure => installed,
  }
  
  package { 'clamav-daemon':
    ensure => installed,
  } ~>
  exec { '/usr/bin/freshclam':
    refreshonly => true,
    timeout     => 0, # this will take a while
  } ->
  service { 'clamav-daemon':
    ensure  => running,
    enable  => true,
  }

  apt::source { 'ubuntu_archive_multiverse':
    location => 'http://us-east-1.ec2.archive.ubuntu.com/ubuntu/',
    release  => $::lsbdistcodename,
    repos    => 'multiverse',
  }
  
  apt::source { 'ubuntu_archive_updates_multiverse':
    location => 'http://us-east-1.ec2.archive.ubuntu.com/ubuntu/',
    release  => "${::lsbdistcodename}-updates",
    repos    => 'multiverse',
  }
  
  package { ['arj',
             'bzip2',
             'cabextract',
             'cpio',
             'file',
             'gzip',
             'lhasa',
             'nomarch',
             'pax',
             'rar',
             'unrar',
             'unzip',
             'zip',
             'zoo']:
    ensure  => installed,
    require => [Apt::Source['ubuntu_archive_multiverse'], Apt::Source['ubuntu_archive_updates_multiverse']],
  }
  
  user { 'amavis':
    groups  => ['amavis', 'clamav'],
    require => [Package['amavisd-new'], Package['clamav-daemon']],
  }
  
  user { 'clamav':
    groups  => ['amavis', 'clamav'],
    require => [Package['amavisd-new'], Package['clamav-daemon']],
  }
  
  service { 'amavis':
    ensure  => running,
    enable  => true,
    require => Package['amavisd-new'],
  }
  
  $secondary_domains = hiera_array('secondary-domains')
  
  mail::amavis::config { '15-content_filter_mode': }
  mail::amavis::config { '50-user':
    template => true,
  }
  
  package { 'razor':
    ensure  => installed,
    require => Package['amavisd-new'],
  } ~>
  exec { '/usr/bin/razor-admin -home=/var/lib/amavis/.razor -create':
    refreshonly => true,
    user        => 'amavis',
  } ~>
  exec { '/usr/bin/razor-admin -home=/var/lib/amavis/.razor -register':
    refreshonly => true,
    user        => 'amavis',
  } ~>
  exec { '/usr/bin/razor-admin -home=/var/lib/amavis/.razor -discover':
    refreshonly => true,
    user        => 'amavis',
  }
  
  package { 'pyzor':
    ensure  => installed,
    require => Package['amavisd-new'],
  } ~>
  exec { '/usr/bin/pyzor --homedir /var/lib/amavis/.pyzor discover':
    refreshonly => true,
    user        => 'amavis',
  }
  
}