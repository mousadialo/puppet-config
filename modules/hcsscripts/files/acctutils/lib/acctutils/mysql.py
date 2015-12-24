import logging
import sys
import MySQLdb, re, hcs
import settings.mysql as settings
import mailers
import models

logger = logging.getLogger('mysql')

class Mysql(object):
    backtick_re = re.compile('^`(.*)`$')

    def __init__(self):
        try:
            self.con = MySQLdb.connect(host=settings.host, user=settings.user, passwd=settings.passwd, db='mysql')
        except MySQLdb.Error, e:
            print "Error %d: %s" % (e.args[0], e.args[1])
            sys.exit (1)

    def query_to_hash(self, query, *args, **kwargs):
        cursor = self.con.cursor(MySQLdb.cursors.DictCursor)
        query = self.substitute_query(query, *args, **kwargs)
        logger.info('About to execute "%s"' % query)
        cursor.execute(query)
        return cursor

    def query(self, query, *args, **kwargs):
        cursor = self.con.cursor()
        try:
            safequery = kwargs['safequery']
        except KeyError:
            safequery = query
        query = self.substitute_query(query, *args, **kwargs)
        safequery = self.substitute_query(safequery, *args, **kwargs)
        logger.info('About to execute "%s"' % safequery)
        cursor.execute(query)
        return cursor

    def substitute_query(self, query, *args, **kwargs):
        import copy
        if args:
            vals = args
            if isinstance(vals[0], dict):
                vals = vals[0]
        else:
            vals = kwargs
        vals = copy.copy(vals)

        if isinstance(vals, str):
            vals = self.escape(vals)
        elif isinstance(vals, dict):
            for k,v in vals.iteritems():
                vals[k] = self.escape(str(v))
        else:
            vals = tuple([self.escape(str(arg)) for arg in vals])
        query = query % vals
        return query

    def grant(self, username, priv='ALL PRIVILEGES', database=None):
        """Grant privilege(s) to a MySQL user."""
        if not database:
            database = username
        database = self.backtick(database)
        if isinstance(priv, str):
            priv = [priv]
        priv = ', '.join(priv)
        for host in settings.user_hosts:
            self.query("GRANT %(priv)s ON %(database)s.* TO %(username)s@%(host)s",
                       {'priv':priv, 'database':database, 'username':username, 'host':host})
        self.query("FLUSH PRIVILEGES")

    def exists(self, database): 
        """Returns true if a database exists"""

        database = self.backtick(database)
        try:
            self.query("SHOW TABLES IN %s", database)
        except MySQLdb.Error, e:
            return False
        return True

    def create(self, username, password, database=None, owner=None):
        """Create MySQL for a user"""
        
        logger.info('Creating database %s for %s (accessible by user %s, owned by %s)' % (database, username, username, owner))
        if not database:
            database = username
        if not password:
            password = hcs.utils.randstring(settings.rand_pw_length)
            logger.info("Random password chosen")
        database = self.backtick(database)
        try:
            self.query("CREATE DATABASE %s", database)
        except Exception, e:
            logger.warn("Could not create database %s: %s" % (database, e))
        else:
            logger.info("Create database %s" % database)

        q = """
GRANT ALL PRIVILEGES ON %(database)s.* TO %(username)s@%(host)s
IDENTIFIED BY %(password)s
"""
        s = """
GRANT ALL PRIVILEGES ON %(database)s.* TO %(username)s@%(host)s
IDENTIFIED BY '***'
"""
        for host in settings.user_hosts:
            logger.info("Creating user '%s'@%s" % (username, host))
            self.query(q, {'username':username, 'password':password, 'host':host, 'database':database}, safequery=s)
        self.query("FLUSH PRIVILEGES")

        try:
            if owner:
                # Check if owner username was passed
                if isinstance(owner, str):
                    owner = models.User.find_by_username(owner)
                    # Notify!
                    mailers.MysqlMailer.deliver_creation(owner, username, password, self.unbacktick(database))
        except Exception, e:
            logger.warn("Could not send notification email: %s" % e)

        try:
            mailers.UserMailer.deliver_acctserv_notification(owner, 'mysql_database_creation', database=self.unbacktick(database))
        except Exception, e:
            logger.warn("Could not notify systems@hcs: %s" % e)

    def change_password(self, username, password=None, owner=None):
        """ Change MySQL password """

        if not password:
            password = hcs.utils.randstring(settings.rand_pw_length)
            logger.info("Random password chosen")
        result = self.query_to_hash("SELECT * FROM user WHERE User = %s", username)
        for row in result.fetchall():
            host = row["Host"]
            logger.info("Setting password for  %s@%s", username, host)
            self.query("SET PASSWORD FOR %s@%s = PASSWORD(%s)", username, host, password)
            try:
                if owner:
                    if isinstance(owner, str):
                        owner = models.User.find_by_username(owner)
                        mailers.MysqlMailer.deliver_newpassword(owner, username, password)
            except Exception, e:
                logger.warn("Could not send notification email: %s" % e)
    
    def escape(self, value):
        if isinstance(value, str):
            value = self.con.escape_string(value)
            if not re.search(self.backtick_re, value):
                value = "'%s'" % value
        else:
            value = self.con.escape_string(value)
        return value

    def backtick(self, value):
        if not re.search(self.backtick_re, value):
            value = '`%s`' % value
        return value

    def unbacktick(self, value):
        match = re.search(self.backtick_re, value)
        if match:
            return match.group(1)
        else:
            return value
