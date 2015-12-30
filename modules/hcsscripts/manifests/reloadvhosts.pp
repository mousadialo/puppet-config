# Configures reloadvhosts cron script.
class hcsscripts::reloadvhosts {

  if $::machine_type == 'web' {
    file { '/etc/cron.hourly/reload-vhosts':
      ensure => file,
      source => 'puppet:///modules/hcsscripts/reloadvhosts/cron.hourly/reload-vhosts',
      owner  => 'root',
      group  => 'root',
      mode   => '0755',
    }
  }

}