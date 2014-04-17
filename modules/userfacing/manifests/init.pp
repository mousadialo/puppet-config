# Userfacing packages available to all HCS users
class userfacing {

  $userfacing = hiera_array('userfacing_packages')
  package { $userfacing:
    ensure => installed
  }

}
