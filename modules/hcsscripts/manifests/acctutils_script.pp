define hcsscripts::acctutils_script($file_name = $title) {

  file { "/usr/bin/$file_name":
    ensure  => file,
    source  => "puppet:///modules/hcsscripts/acctutils/bin/$file_name",
    owner   => 'root',
    group   => 'root',
    mode    => '0755',
  }

}