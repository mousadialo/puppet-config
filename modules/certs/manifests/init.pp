# manages certificates on the machine
class certs {

  $chain         = '/etc/ssl/certs/hcs_harvard_edu.cer'
  $certificate   = '/etc/ssl/certs/hcs_harvard_edu_cert.cer'
  $intermediates = '/etc/ssl/certs/hcs_harvard_edu_interm.cer'
  $key           = '/etc/ssl/private/hcs_harvard_edu.key'

  # X509 Certificate Chain Reverse, Base64 encoded
  # it's simply the concatenation of the certificate and intermediates files
  concat { $chain:
    ensure         => present,
    owner          => 'root',
    group          => 'root',
    mode           => '0444',
    ensure_newline => true,
  }
  concat::fragment { $certificate:
    target => $chain,
    source => 'puppet:///modules/web/certs/hcs_harvard_edu_cert.cer',
    order  => '1',
  }
  concat::fragment { $intermediates:
    target => $chain,
    source => 'puppet:///modules/web/certs/hcs_harvard_edu_interm.cer',
    order  => '2',
  }

  # X509 Certificate only, Base64 encoded
  file { $certificate:
    ensure => file,
    source => 'puppet:///modules/web/certs/hcs_harvard_edu_cert.cer',
    owner  => 'root',
    group  => 'root',
    mode   => '0444',
  }

  # X509 Intermediates/root only Reverse, Base64 encoded
  file { $intermediates:
    ensure => file,
    source => 'puppet:///modules/web/certs/hcs_harvard_edu_interm.cer',
    owner  => 'root',
    group  => 'root',
    mode   => '0444',
  }

  # HCS super private key
  file { $key:
    ensure  => file,
    content => hiera('hcs_harvard_edu.key'),
    owner   => 'root',
    group   => 'root',
    mode    => '0400',
  }

}