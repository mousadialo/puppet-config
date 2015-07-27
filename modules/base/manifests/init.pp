# Base configuration for HCS machines
class base {
  include base::timezone
  include base::users
  include base::motd

  package { 'build-essential':
    ensure => installed,
  }
  
#   # install the base packages listed in data/ubuntu.yaml
#   $packages_ubuntu = hiera_array('base_packages')
#   package { $packages_ubuntu:
#     ensure => installed,
#   }

}
