import hcs.passwords, os

dir = os.path.dirname(__file__)
ldap_uri = 'ldaps://hcs.harvard.edu:389'
try:
    ldap_directory_manager = 'cn=Directory Manager'
    ldap_password = hcs.passwords.get('ldap')
except hcs.passwords.PasswordNotFound:
    ldap_directory_manager = ''
    ldap_password = ''
ldap_base_dn = 'dc=hcs,dc=harvard,dc=edu'
