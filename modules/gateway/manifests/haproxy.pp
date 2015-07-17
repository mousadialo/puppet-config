# configuration for haproxy
class gateway::haproxy {

  class { 'haproxy':
    defaults_options => {
      'log'     => 'global',
      'stats'   => 'enable',
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
    peers_name => 'bifrost',
    port       => '1039',
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
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'httpchk',
    },
  }
  
  haproxy::backend { 'web-https':
    options => {
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'ssl-hello-chk',
    },
  }
  
  haproxy::backend { 'lists-http':
    options => {
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'httpchk',
    },
  }
  
  haproxy::backend { 'lists-https':
    options => {
      'balance'   => 'source',
      'hash-type' => 'consistent',
      'option'    => 'ssl-hello-chk',
    },
  }
}
