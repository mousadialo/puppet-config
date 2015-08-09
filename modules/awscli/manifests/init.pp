# Installs and configures the AWS CLI
class awscli {

  package { 'python-pip':
    ensure => installed,
  } ->
  package { 'awscli':
    ensure   => installed,
    provider => pip,
  }

}
  