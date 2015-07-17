# configuration for bifrost servers
class gateway {

  include apt

  apt::ppa { 'ppa:vbernat/haproxy-1.5':
    package_manage => false,
  } ->
  class { 'haproxy':
    defaults_options => {
      'log'     => 'global',
      'option'  => 'redispatch',
      'retries' => '3',
      'timeout' => [
        'http-request 5s',
        'queue        30s',
        'connect      5s',
        'client       30s',
        'client-fin   30s',
        'server       30s',
        'check        10s',
        'tunnel       1h',
      ],
      'maxconn' => '8000',
    }
  }

  haproxy::peers { 'bifrost': }
  
  @@haproxy::peer { "${::hostname}-peer":
    peers_name   => 'bifrost',
    server_names => $::hostname,
    ipaddresses  => $::ipaddress,
    port         => '1039',
  }
  
  haproxy::frontend { 'http':
    ports   => '80',
    mode    => 'http',
    options => {
      'acl'             => 'host_lists hdr(host) -i lists.hcs.harvard.edu lists.hcs.so',
      'use_backend'     => 'lists-http if host_lists',
      'default_backend' => 'web-http',
    },
  }

  haproxy::frontend { 'https':
    ports   => '443',
    mode    => 'tcp',
    options => {
      'acl'             => 'sni_lists req_ssl_sni -i lists.hcs.harvard.edu lists.hcs.so',
      'use_backend'     => 'lists-https if sni_lists',
      'default_backend' => 'web-https',
    },
  }
  
  haproxy::backend { 'web-http':
    options => {
      'mode'      => 'http',
      'stats'     => [
        'enable',
        'uri /stats',
        'hide-version',
        'auth hcs:euph4ria'
      ]
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'httpchk',
    },
  }
  
  haproxy::backend { 'web-https':
    options => {
      'mode'      => 'tcp',
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'ssl-hello-chk',
    },
  }
  
  haproxy::backend { 'lists-http':
    options => {
      'mode'      => 'http',
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'httpchk',
    },
  }
  
  haproxy::backend { 'lists-https':
    options => {
      'mode'      => 'tcp',
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'ssl-hello-chk',
    },
  }

}
