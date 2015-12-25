import os, logging, time, models, driver
import mailers
logger = logging.getLogger('observers')

class UserObserver(object):
#     @staticmethod
#     def clear_cache(user):
#         user.invalidate_cache()

    @staticmethod
    def after_create(user):
        if user.name is not None:
            name = ' (name: %s)' % user.name
        else:
            name = ''
        logging.info("Created a new user %s%s of type %s, adding to gid %d" % (user.username, name, user.type, user.gid))
        group = models.Group.find_by_gid(user.gid)
        try:
            group.members.append(user.username)
        except AttributeError:
            if group.members:
                group.members = [group.members, user.username]
            else:
                group.members = [user.username]
        group.save()
        logging.info("Creating filesystem %s" % user.home)
        driver.run(['/usr/bin/hcs-quota', 'create', user.username, user.quota])
        if not os.path.exists(user.home):
            logger.error("Homedir doesn't look right here. NFS issue?")
            raise AssertionError
        user.take_file(user.home)
        driver.run(['/usr/bin/new-cert', '-u', user.username])
        user.create_initial_files()
        logging.debug("Created initial files, user.type = %s" % user.type)
        # Let the cache on the mailservers get up to speed
        time.sleep(1.2)
        if user.type == 'group':
            # Goes to the account + access list
            mailers.UserMailer.deliver_creation(user, access_list=True)
            # Goes to the account
            mailers.UserMailer.deliver_www_info(user, access_list=False)
            mailers.UserMailer.deliver_user_info(user, access_list=False)
        elif user.type == 'member':
            if not user.state('no_welcome_email'):
                logging.debug("About to send member welcome")
                mailers.UserMailer.deliver_member_welcome(user, outside=False)
            else:
                logging.debug("Opted out of member welcome")
        elif user.type == 'general':
            # TODO: write a more specific email
            if not user.state('no_welcome_email'):
                logging.debug("About to send general welcome.  TODO: make it better")
                mailers.UserMailer.deliver_member_welcome(user, outside=False)
            else:
                logging.debug("Opted out of general welcome")
        mailers.UserMailer.deliver_acctserv_notification(user, 'creation')

    @staticmethod
    def after_save(user):
        if user.type == 'group' and (user.state('just-created') or
                                     user.changed('access_list_emails')):
            try:
                access_list = user.access_list
            except AttributeError:
                pass
            else:
                access_list.save()
