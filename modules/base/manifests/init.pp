# Base configuration for HCS machines
class base {
  include base::timezone
  include base::users
  include base::alpine

  # install the base packages listed in data/hcs.yaml
/*
  $packages_userfacing = hiera_array('userfacing_packages')
  package { $packages_userfacing:
    ensure => installed
  }

  # install the base packages listed in data/ubuntu.yaml
  $packages_ubuntu = hiera_array('base_packages')
  package { $packages_ubuntu:
    ensure => installed
  }
*/

}
