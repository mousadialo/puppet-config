# HCS MOTD configuration
class base::motd {

  # Remove Ubuntu documentation
  file { '/etc/update-motd.d/10-help-text':
    mode => '0644'
  }

  # Remove Ubuntu advertisement
  file { '/etc/update-motd.d/51-cloudguest':
    ensure => absent,
  }
  
  file { '/etc/update-motd.d/99-footer':
    ensure => file,
    source => 'puppet:///modules/base/motd/99-footer'
    owner  => 'root',
    group  => 'root',
    mode   => '0755',
  }
  
  file { '/etc/motd.tail':
    ensure => file,
    source => template('base/motd/motd.tail.erb')
    owner  => 'root',
    group  => 'root',
    mode   => '0644',
  }

  # Remove Landscape advertisement
  file { '/etc/landscape/client.conf':
    ensure => file,
    source => 'puppet:///modules/base/motd/client.conf',
    owner  => 'landscape',
    group  => 'root',
    mode   => '0600',
  }
  
}