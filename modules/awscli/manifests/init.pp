# Installs and configures the AWS CLI
class awscli {

  package { 'awscli':
    ensure   => installed,
    provider => pip,
  }

}
  