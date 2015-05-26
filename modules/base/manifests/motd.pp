# HCS MOTD configuration
class base::motd {

  # Remove Ubuntu documentation
  file { '/etc/update-motd.d/10-help-text':
    mode => '0644'
  }

  # Remove Ubuntu advertisement
  file { '/etc/update-motd.d/51-cloudguest':
    mode => '0644'
  }

  # Remove Landscape advertisement
  file { '/etc/landscape/client.conf':
    ensure => file,
    source => 'puppet:///modules/base/landscape/client.conf',
    owner  => 'landscape',
    group  => 'root',
    mode   => '0600',
  }
  
  #TODO Add HCS Banner
  if $::machine_type == 'login' {
  
  }
  
}