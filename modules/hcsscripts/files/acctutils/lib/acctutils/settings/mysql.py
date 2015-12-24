import hcs.passwords, os

host = "mysql.hcs.harvard.edu"
user = "hcs"
passwd = hcs.passwords.get('mysql')
user_hosts = ['%']
rand_pw_length = 12
