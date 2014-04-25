# Base configuration for HCS machines
class base {
  include base::timezone
  include base::users
  include base::alpine

  # install the base packages listed in data/hcs.yaml
  $packages = hiera_array('userfacing_packages')
#  package { $packages:
    #ensure => installed
  #}

  # install the base packages listed in data/Ubuntu.yaml
  #$packages_ubuntu = hiera_array('base_packages')
  #package { $packages:
    #ensure => installed
  #}

}
