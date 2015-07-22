# configuration for bifrost servers
class gateway {

  require sshd
  include apt
  
  apt::ppa { 'ppa:vbernat/haproxy-1.5':
    package_manage => false,
  }

  $pem = '/etc/haproxy/hcs_harvard_edu.pem'
  # PEM file containing everything
  # it's the concatenation of the certificate, intermediates file, and key file
  concat { $pem:
    ensure         => present,
    owner          => 'haproxy',
    group          => 'haproxy',
    mode           => '0400',
    ensure_newline => true,
  }
  concat::fragment { "${pem}-certificate":
    target => $pem,
    source => 'puppet:///modules/certs/hcs_harvard_edu_cert.cer',
    order  => '1',
  }
  concat::fragment { "${pem}-intermediates":
    target => $pem,
    source => 'puppet:///modules/certs/hcs_harvard_edu_interm.cer',
    order  => '2',
  }
  concat::fragment { "${pem}-key":
    target  => $pem,
    content => hiera('hcs_harvard_edu.key'),
    order   => '3',
  }
  
  class { 'haproxy':
    global_options => {
      'log'     => [
        '/dev/log local0',
        '/dev/log local1 notice',
      ],
      'chroot'                    => '/var/lib/haproxy',
      'pidfile'                   => '/var/run/haproxy.pid',
      'maxconn'                   => '4000',
      'user'                      => 'haproxy',
      'group'                     => 'haproxy',
      'daemon'                    => '',
      'stats'                     => 'socket /var/lib/haproxy/stats',
      'tune.ssl.default-dh-param' => '2048',
    },
    defaults_options => {
      'log'            => 'global',
      'option'         => 'redispatch',
      'retries'        => '3',
      'timeout'        => [
        'http-request 5s',
        'queue        30s',
        'connect      5s',
        'client       30s',
        'client-fin   30s',
        'server       5m',
        'check        10s',
        'tunnel       1h',
      ],
      'maxconn'        => '8000',
      'default-server' => 'inter 30000 fastinter 10000 downinter 3000'
    },
    require => [Apt::Ppa['ppa:vbernat/haproxy-1.5'], Concat[$pem]],
  }

  haproxy::peers { 'bifrost': }
  
  @@haproxy::peer { "${::hostname}-peer":
    peers_name   => 'bifrost',
    server_names => $::fqdn,
    ipaddresses  => $::ipaddress,
    port         => '3009',
  }
  
  haproxy::frontend { 'http':
    bind    => {
      '*:80' => [],
    },
    mode    => 'http',
    options => {
      'acl'             => 'host_lists hdr(host) -i lists.hcs.harvard.edu lists.hcs.so',
      'use_backend'     => 'lists-http if host_lists',
      'default_backend' => 'web-http',
    },
  }
  
  haproxy::backend { 'web-http':
    options => {
      'mode'    => 'http',
      'balance' => 'roundrobin',
      'cookie'  => 'SRV insert indirect nocache',
      'option'  => 'forwardfor',
      #'option'  => 'httpchk',
    },
  }
  
  haproxy::backend { 'lists-http':
    options => {
      'mode'    => 'http',
      'balance' => 'roundrobin',
      'cookie'  => 'SRV insert indirect nocache',
      'option'  => 'forwardfor',
      #'option'  => 'httpchk',
    },
  }

  haproxy::frontend { 'https':
    bind    => {
      '*:443' => ['ssl', 'crt', $pem]
    },
    mode    => 'http',
    options => {
      'acl'             => 'host_lists hdr(host) -i lists.hcs.harvard.edu lists.hcs.so',
      'use_backend'     => 'lists-https if host_lists',
      'default_backend' => 'web-https',
    },
  }
  
  haproxy::backend { 'web-https':
    options => {
      'mode'    => 'http',
      'balance' => 'roundrobin',
      'cookie'  => 'SRV insert indirect nocache',
      'option'  => 'forwardfor',
      #'option'  => 'httpchk',
      'stats'   => [
        'enable',
        'uri /admin?stats',
        'hide-version',
        'auth hcs:euph4ria',
      ],
    },
  }
  
  haproxy::backend { 'lists-https':
    options => {
      'mode'    => 'http',
      'balance' => 'roundrobin',
      'cookie'  => 'SRV insert indirect nocache',
      'option'  => 'forwardfor',
      #'option'  => 'httpchk',
    },
  }
  
  haproxy::listen { 'mail-smtp':
    bind    => {
      '*:25' => [],
    },
    mode    => 'tcp',
    options => {
      #'option' => 'smtpchk'
    },
  }
  
  haproxy::listen { 'mail-imaps':
    bind    => {
      '*:993' => [],
    },
    mode    => 'tcp',
    options => {
      #'option' => 'ssl-hello-chk',
    },
  }
  
  haproxy::listen { 'mail-pop3s':
    bind    => {
      '*:995' => [],
    },
    mode    => 'tcp',
    options => {
      #'option' => 'ssl-hello-chk',
    },
  }
  
  haproxy::listen { 'mail-smtp-vrfy':
    bind    => {
      '*:10025' => [],
    },
    mode    => 'tcp',
    options => {
      'acl'         => 'mynetworks src 10.0.0.0/8',
      'tcp-request' => 'content reject if !mynetworks',
      #'option'      => 'smtpchk',
    },
  }
  
  haproxy::listen { 'lists-smtp':
    bind    => {
      '*:2525' => [],
    },
    mode    => 'tcp',
    options => {
      #'option' => 'smtpchk'
    },
  }
  
  haproxy::frontend { 'ssh':
    bind    => {
      '*:22' => [],
    },
    mode => 'tcp',
    options => {
      'acl'             => 'mynetworks src 127.0.0.0/8 10.0.0.0/8',
      'use_backend'     => 'login-ssh if !mynetworks',
      'default_backend' => 'local-ssh',
    },
  }
  
  haproxy::backend { 'login-ssh':
    options => {
      'mode'      => 'tcp',
      'balance'   => 'source',
      #'option'    => 'tcp-check',
      #'tcp-check' => 'expect string SSH-2.0-'
    },
  }
  
  haproxy::backend { 'local-ssh':
    collect_exported => false,
    options          => {
      'mode' => 'tcp',
    },
  }
  
  # Connect to the local SSH Daemon which is listening on port 2222 if
  # connection originates from the private network.
  haproxy::balancermember { 'local-ssh':
    listening_service => 'local-ssh',
    server_names      => $::fqdn,
    ipaddresses       => '127.0.0.1',
    ports             => ['2222'],
  }
  
  haproxy::listen { 'ldap':
    bind    => { 
      '*:389' => [],
    },
    mode    => 'tcp',
    options => {
      'acl'         => 'mynetworks src 10.0.0.0/8',
      'tcp-request' => 'content reject if !mynetworks',
    },
  }
  
  haproxy::listen { 'makelist':
    bind    => { 
      '*:8080' => [],
    },
    mode    => 'tcp',
    options => {
      'acl'         => 'mynetworks src 10.0.0.0/8',
      'tcp-request' => 'content reject if !mynetworks',
    },
  }
  
}
