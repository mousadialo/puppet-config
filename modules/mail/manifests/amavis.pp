# AMaViS configuration
class mail::amavis {

  apt::source { 'ubuntu_archive_multiverse':
    location => 'http://us-east-1.ec2.archive.ubuntu.com/ubuntu/',
    release  => $lsbdistcodename,
    repos    => 'multiverse',
  }
  
  apt::source { 'ubuntu_archive_updates_multiverse':
    location => 'http://us-east-1.ec2.archive.ubuntu.com/ubuntu/',
    release  => "${lsbdistcodename}-updates",
    repos    => 'multiverse',
  }

  package { ['amavisd-new',
             'spamassassin',
             'clamav-daemon',
             'libnet-dns-perl',
             'libmail-spf-perl']:
    ensure => installed,
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
  
  service { 'amavis':
    ensure  => running,
    enable  => true,
    require => Package['amavisd-new'],
  }
  
  mail::amavis::config { '15-content_filter_mode': }
  mail::amavis::config { '50-user': }
  
  user { 'amavis':
    groups  => ['amavis', 'clamav'],
    require => [Package['amavisd-new'], Package['clamav-daemon']],
  }
  
  user { 'clamav':
    groups  => ['amavis', 'clamav'],
    require => [Package['amavisd-new'], Package['clamav-daemon']],
  }
  
  package { 'razor':
    ensure => installed,
  } ~>
  exec { '/usr/bin/razor-admin -create':
    refreshonly => true,
    user        => 'amavis',
  } ~>
  exec { '/usr/bin/razor-admin -register':
    refreshonly => true,
    user        => 'amavis',
  } ~>
  exec { '/usr/bin/razor-admin -discover':
    refreshonly => true,
    user        => 'amavis',
  }
  
  package { 'pyzor':
    ensure => installed,
  } ~>
  exec { '/usr/bin/pyzor discover':
    refreshonly => true,
    user        => 'amavis',
  }
  
}