import os

host = 'hcs.harvard.edu'
notification_email = 'hcs-systems-bots@hcs.harvard.edu'
send_from = 'acctserv@hcs.harvard.edu'
templates_base = '/etc/hcs/mail_templates'
layouts_dir = os.path.join(templates_base, 'layouts')
