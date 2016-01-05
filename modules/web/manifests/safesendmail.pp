# Configures safesendmail to rate limit PHP mails.
class web::safesendmail {

  $hcs_sendmail_mysql_password = hiera('hcs-sendmail-mysql-password')
  file { '/usr/sbin/safesendmail':
    ensure  => file,
    content => template('web/safesendmail/safesendmail'),
    owner   => 'root',
    group   => 'root',
    mode    => '0755',
  }

}