class default_node {
	package { 'apache2':
		ensure => installed
	}
	service { 'apache2':
		ensure => true,
		enable => true,
		require => Package['apache2']
	}
}
