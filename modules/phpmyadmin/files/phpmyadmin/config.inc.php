// Load secret generated on postinst
include('/var/lib/phpmyadmin/blowfish_secret.inc.php');

/**
 * Server(s) configuration
 */
$i = 0;
$i++;

$cfg['Servers'][$i]['auth_type'] = 'http';
$cfg['Servers'][$i]['hide_db'] = '(mysql|information_schema|phpmyadmin)';
    /* Server parameters */
$cfg['Servers'][$i]['host'] = 'mysql.c8dgncickp9k.us-east-1.rds.amazonaws.com';
$cfg['Servers'][$i]['AllowNoPassword'] = false;
