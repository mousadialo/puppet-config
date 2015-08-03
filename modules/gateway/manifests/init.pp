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
    require        => Package['haproxy'],
    before         => Service['haproxy'],
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
  
  exec{ 'retrieve_cloudflare_ips':
    command => '/usr/bin/wget -q https://www.cloudflare.com/ips-v4 -O /etc/haproxy/cloudflare_ips',
    creates => '/etc/haproxy/cloudflare_ips',
    require => Package['haproxy'],
  } ->
  file { '/etc/haproxy/cloudflare_ips':
    owner   => 'haproxy',
    group   => 'haproxy',
    mode    => '0644',
    before  => Service['haproxy'],
  }
  
  file { '/etc/haproxy/harvard_ips':
    ensure  => file,
    source  => 'puppet:///modules/gateway/harvard_ips',
    owner   => 'haproxy',
    group   => 'haproxy',
    mode    => '0644',
    require => Package['haproxy'],
    before  => Service['haproxy'],
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
      'spread-checks'             => '5',
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
      'default-server' => 'inter 30000 fastinter 10000 downinter 3000',
    },
    restart_command  => '/usr/sbin/service haproxy reload',
    require => Apt::Ppa['ppa:vbernat/haproxy-1.5'],
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
      'stick-table'            => 'type ip size 300k expire 30s peers bifrost store gpc0',
      'acl'                    => [
        'host_lists hdr(host) -i lists.hcs.harvard.edu lists.hcs.so',
        'mynetworks src 127.0.0.0/8 10.0.0.0/8',
        'harvard src -f /etc/haproxy/harvard_ips',
        'cloudflare src -f /etc/haproxy/cloudflare_ips',
        'blacklisted sc0_get_gpc0(http) gt 0',
      ],
      'tcp-request connection' => [
        'accept if mynetworks or harvard or cloudflare',
        'track-sc0 src table http',
      ],
      'http-request'           => [
        'allow if mynetworks or harvard or cloudflare',
        'tarpit if blacklisted',
      ],
      'monitor-uri'            => '/health',
      'use_backend'            => 'lists-http if host_lists',
      'default_backend'        => 'web-http',
    },
  }
  
  haproxy::backend { 'web-http':
    options => {
      'mode'                => 'http',
      'balance'             => 'roundrobin',
      'cookie'              => 'SRV insert indirect nocache',
      'stick-table'         => 'type ip size 200k expire 15s peers bifrost store conn_cur,conn_rate(3s),http_req_rate(15s),http_err_rate(15s)',
      'acl'                 => [
        'high_conn_cur sc1_conn_cur(web-http) gt 10',
        'high_conn_rate sc1_conn_rate(web-http) gt 20',
        'high_req_rate sc1_http_req_rate(web-http) gt 100',
        'high_err_rate sc1_http_err_rate(web-http) gt 20',
        
        'wp_login path_end -i /wp-login.php',
        'bruteforce_detection sc2_http_req_rate(web-https) gt 5',
        
        'blacklist sc0_inc_gpc0(http) gt 0',
      ],
      'tcp-request content' => [
        'reject if high_conn_cur',
        'reject if high_conn_rate',
        'track-sc1 src table web-http',
        'track-sc2 src table web-https if METH_POST wp_login',
      ],
      'http-request'        => [
        'deny if high_req_rate blacklist',
        'deny if high_err_rate blacklist',
        'deny if bruteforce_detection blacklist',
      ],
      'option'              => [
        'forwardfor',
        'httpchk HEAD /health HTTP/1.1\r\nHost:www.hcs.harvard.edu',
        'httplog',
      ],
    },
  }
  
  haproxy::backend { 'lists-http':
    options => {
      'mode'                => 'http',
      'balance'             => 'roundrobin',
      'cookie'              => 'SRV insert indirect nocache',
      'stick-table'         => 'type ip size 200k expire 15s peers bifrost store conn_cur,conn_rate(3s),http_req_rate(15s),http_err_rate(15s)',
      'acl'                 => [
        'high_conn_cur sc1_conn_cur(lists-http) gt 10',
        'high_conn_rate sc1_conn_rate(lists-http) gt 20',
        'high_req_rate sc1_http_req_rate(lists-http) gt 100',
        'high_err_rate sc1_http_err_rate(lists-http) gt 20',
        
        'blacklist sc0_inc_gpc0(http) gt 0',
      ],
      'tcp-request content' => [
        'reject if high_conn_cur',
        'reject if high_conn_rate',
        'track-sc1 src table lists-http',
      ],
      'http-request'        => [
        'deny if high_req_rate blacklist',
        'deny if high_err_rate blacklist',
      ],
      'option'              => [
        'forwardfor',
        'httpchk',
        'httplog',
      ],
    },
  }

  haproxy::frontend { 'https':
    bind    => {
      '*:443' => ['ssl', 'crt', $pem]
    },
    mode    => 'http',
    options => {
      'acl'                    => [
        'host_lists hdr(host) -i lists.hcs.harvard.edu lists.hcs.so',
        'mynetworks src 127.0.0.0/8 10.0.0.0/8',
        'harvard src -f /etc/haproxy/harvard_ips',
        'cloudflare src -f /etc/haproxy/cloudflare_ips',
        'blacklisted sc0_get_gpc0(http) gt 0',
      ],
      'tcp-request connection' => [
        'accept if mynetworks or harvard or cloudflare',
        'track-sc0 src table http',
      ],
      'http-request'           => [
        'allow if mynetworks or harvard or cloudflare',
        'tarpit if blacklisted',
      ],
      'monitor-uri'            => '/health',
      'use_backend'            => 'lists-https if host_lists',
      'default_backend'        => 'web-https',
    },
  }
  
  $stats_pwd = hiera('stats_pwd')
  haproxy::backend { 'web-https':
    options => {
      'mode'                => 'http',
      'balance'             => 'roundrobin',
      'cookie'              => 'SRV insert indirect nocache',
      'stick-table'         => 'type ip size 200k expire 5m peers bifrost store http_req_rate(5m)',
      'acl'                 => [
        'high_conn_cur sc1_conn_cur(web-http) gt 10',
        'high_conn_rate sc1_conn_rate(web-http) gt 20',
        'high_req_rate sc1_http_req_rate(web-http) gt 100',
        'high_err_rate sc1_http_err_rate(web-http) gt 20',
        
        'wp_login path_end -i /wp-login.php',
        'bruteforce_detection sc2_http_req_rate(web-https) gt 5',
        
        'blacklist sc0_inc_gpc0(http) gt 0',
      ],
      'tcp-request content' => [
        'reject if high_conn_cur',
        'reject if high_conn_rate',
        'track-sc1 src table web-http',
        'track-sc2 src table web-https if METH_POST wp_login',
      ],
      'http-request'        => [
        'deny if high_req_rate blacklist',
        'deny if high_err_rate blacklist',
        'deny if bruteforce_detection blacklist',
      ],
      'option'              => [
        'forwardfor',
        'httpchk HEAD /health HTTP/1.1\r\nHost:www.hcs.harvard.edu',
        'httplog',
      ],
      'stats'               => [
        'enable',
        'realm HAProxy\ Statistics',
        'uri /admin?stats',
        'hide-version',
        "auth hcs:${stats_pwd}",
      ],
    },
  }
  
  haproxy::backend { 'lists-https':
    options => {
      'mode'                => 'http',
      'balance'             => 'roundrobin',
      'cookie'              => 'SRV insert indirect nocache',
      'acl'                 => [
        'high_conn_cur sc1_conn_cur(lists-http) gt 10',
        'high_conn_rate sc1_conn_rate(lists-http) gt 20',
        'high_req_rate sc1_http_req_rate(lists-http) gt 100',
        'high_err_rate sc1_http_err_rate(lists-http) gt 20',
        
        'blacklist sc0_inc_gpc0(http) gt 0',
      ],
      'tcp-request content' => [
        'reject if high_conn_cur',
        'reject if high_conn_rate',
        'track-sc1 src table lists-http',
      ],
      'http-request'        => [
        'deny if high_req_rate blacklist',
        'deny if high_err_rate blacklist',
      ],
      'option'              => [
        'forwardfor',
        'httpchk',
        'httplog',
      ],
    },
  }
  
  haproxy::listen { 'mail-smtp':
    bind    => {
      '*:25' => [],
    },
    mode    => 'tcp',
    options => {
      'balance' => 'leastconn',
      'option'  => [
        'smtpchk',
        'tcplog',
      ],
    },
  }
  
  haproxy::listen { 'mail-imaps':
    bind    => {
      '*:993' => [],
    },
    mode    => 'tcp',
    options => {
      'balance' => 'leastconn',
      'option'  => [
        'tcp-check',
        'tcplog',
      ],
    },
  }
  
  haproxy::listen { 'mail-pop3s':
    bind    => {
      '*:995' => [],
    },
    mode    => 'tcp',
    options => {
      'balance' => 'leastconn',
      'option'  => [
        'tcp-check',
        'tcplog',
      ],
    },
  }
  
  haproxy::listen { 'mail-smtp-relay':
    bind    => {
      '*:10025' => [],
    },
    mode    => 'tcp',
    options => {
      'balance'     => 'leastconn',
      'acl'         => 'mynetworks src 10.0.0.0/8',
      'tcp-request' => 'connection reject if !mynetworks',
      'option'      => [
        'smtpchk',
        'tcplog',
      ],
    },
  }
  
  haproxy::listen { 'lists-smtp':
    bind    => {
      '*:2525' => [],
    },
    mode    => 'tcp',
    options => {
      'balance' => 'leastconn',
      'option'  => [
        'smtpchk',
        'tcplog',
      ],
    },
  }
  
  haproxy::frontend { 'ssh':
    bind    => {
      '*:22' => [],
    },
    mode => 'tcp',
    options => {
      'stick-table'            => 'type ip size 1m expire 1d peers bifrost store conn_cur,conn_rate(1d)',
      'acl'                    => [
        'mynetworks src 127.0.0.0/8 10.0.0.0/8',
        'harvard src -f /etc/haproxy/harvard_ips',
        'high_conn_cur sc0_conn_cur(ssh) ge 10',
        'high_conn_rate sc0_conn_rate(ssh) ge 300',
      ],
      'tcp-request connection' => [
        'accept if mynetworks or harvard',
        'reject if high_conn_cur',
        'reject if high_conn_rate',
        'track-sc0 src table ssh',
      ],
      'use_backend'            => 'local-ssh if mynetworks',
      'default_backend'        => 'login-ssh',
    },
  }
  
  haproxy::backend { 'login-ssh':
    options => {
      'mode'                => 'tcp',
      'balance'             => 'leastconn',
      'stick-table'         => 'type ip size 500k expire 1h peers bifrost',
      'stick on'            => 'src table login-ssh',
      'option'              => [
        'tcp-check',
        'tcplog',
      ],
      'tcp-check'           => 'expect string SSH-2.0-'
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
      'balance'     => 'leastconn',
      'acl'         => 'mynetworks src 127.0.0.0/8 10.0.0.0/8',
      'tcp-request' => 'connection reject if !mynetworks',
      'option'      => [
        'tcp-check',
        'tcplog',
      ],
    },
  }
  
  haproxy::listen { 'ldaps':
    bind    => { 
      '*:636' => [],
    },
    mode    => 'tcp',
    options => {
      'balance'     => 'leastconn',
      'acl'         => 'mynetworks src 127.0.0.0/8 10.0.0.0/8',
      'tcp-request' => 'connection reject if !mynetworks',
      'option'      => [
        'tcp-check',
        'tcplog',
      ],
    },
  }
  
  haproxy::listen { 'makelist':
    bind    => { 
      '*:8080' => [],
    },
    mode    => 'tcp',
    options => {
      'balance'     => 'leastconn',
      'acl'         => 'mynetworks src 127.0.0.0/8 10.0.0.0/8',
      'tcp-request' => 'connection reject if !mynetworks',
      'option'      => [
        'tcp-check',
        'tcplog',
      ],
    },
  }
  
}
