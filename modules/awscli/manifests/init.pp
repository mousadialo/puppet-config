# Installs and configures the AWS CLI
class awscli {

  package { 'python-pip':
    ensure => installed,
  } ->
  package { 'awscli':
    ensure   => installed,
    provider => pip,
  }
  
  $access_key_id = hiera('access-key-id')
  $secret_access_key = hiera('secret-access-key')
  file { '/root/.aws':
    ensure => directory,
    owner  => 'root',
    group  => 'root',
  } ->
  file { '/root/.aws/config':
    ensure  => file,
    content => template('awscli/config.erb'),
    owner   => 'root',
    group   => 'root',
    mode    => '0644',
  }

}
  