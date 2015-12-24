import hcs.passwords

ldap_uri = 'ldaps://ldap.internal.hcs.harvard.edu'
try:
    ldap_directory_manager = 'cn=Directory Manager'
    ldap_password = hcs.passwords.get('ldap')
except hcs.passwords.PasswordNotFound:
    ldap_directory_manager = ''
    ldap_password = ''
ldap_base_dn = 'dc=hcs,dc=harvard,dc=edu'

