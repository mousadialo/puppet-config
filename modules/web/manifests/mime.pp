# MIME configuration
class web::mime {
  
  file_line { 'x-httpd-php':
    path  => '/etc/mime.types',
    line  => 'application/x-httpd-php			phtml pht php',
    match => '^#?application/x-httpd-php\s+',
    notify  => Service['apache2'],
  }
  
  file_line { 'x-httpd-php-source':
    path  => '/etc/mime.types',
    line  => 'application/x-httpd-php-source			phps',
    match => '^#?application/x-httpd-php-source\s+',
    notify  => Service['apache2'],
  }
  
  file_line { 'x-httpd-php3':
    path  => '/etc/mime.types',
    line  => 'application/x-httpd-php3			php3',
    match => '^#?application/x-httpd-php3\s+',
    notify  => Service['apache2'],
  }
  
  file_line { 'x-httpd-php3-preprocessed':
    path  => '/etc/mime.types',
    line  => 'application/x-httpd-php3-preprocessed		php3p',
    match => '^#?application/x-httpd-php3-preprocessed\s+',
    notify  => Service['apache2'],
  }
  
  file_line { 'x-httpd-php4':
    path  => '/etc/mime.types',
    line  => 'application/x-httpd-php4			php4',
    match => '^#?application/x-httpd-php4\s+',
    notify  => Service['apache2'],
  }
  
  file_line { 'x-httpd-php5':
    path  => '/etc/mime.types',
    line  => 'application/x-httpd-php5			php5',
    match => '^#?application/x-httpd-php5\s+',
    notify  => Service['apache2'],
  }

}