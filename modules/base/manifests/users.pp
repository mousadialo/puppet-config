class base::users {

  file { '/local':
    ensure => directory
  } ->
  file { '/local/home':
    ensure => directory
  } ->
  file { '/local/home/hcs':
    owner => 'hcs',
    group => 'root',
    ensure => directory,
  } ->

  user { 'hcs':
    ensure   => present,
    gid      => 'root',
    home     => '/local/home/hcs',
    comment  => 'HCS root user',
    shell    => '/bin/bash',
    password => '9cdd51b6f09b8572c15a24a717827e124b3355cf',
    groups   => ['adm', /*'admin-lite',*/ 'admin'] #TODO remove admin, make admin-lite the gid and add it to to sudoers like on cato
  } ->
  file {
    "/local/home/hcs/.ssh":
      ensure => directory,
      owner => 'hcs',
      group => 'root',
      mode => '700';

    "/local/home/hcs/.ssh/authorized_keys":
      ensure => present,
      owner => 'hcs',
      group => 'root',
      mode => '600',
      content => "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAyu3Ag2EAyQcCrl6QYP/8677H2EeqlIYT2j3RTha+DItH5Q/mg6FLCCX48Lc/qYRjXPrDaYN7SEDajYavHV9goSikmsAOzavM52PiaC9NY2QiDJ/QG50yq/GbKs0wYMTLwWeGNkUwpc/vem7tatdsPpSJKWZ5SajggZApb9e41mjI1SzeN5J8nLQMcpsptdpzk+PPC7jjYdxio30b1EEkgJ1mioEXwhEtOgK40uvETtw7aGADcNNykALA5vUGeSLwsIHHa+HrGZS+Hy2sxp89dboJs3HuYyNhU0ROdR1KoQ8kp7WqMajolESiYZ8vpxOa2+mt/Dcc0BYSs5jKJLwcaw== srinchiera@college.harvard.edu"
  }
}
