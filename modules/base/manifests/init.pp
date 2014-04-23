# Base configuration for HCS machines
class base {
  include base::timezone
  include basee::users

  # install the base packages listed in data/Ubuntu.yaml
  $packages = hiera_array('base_packages')
  package { $packages:
    ensure => installed
  }

}
