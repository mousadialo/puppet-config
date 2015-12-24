import os, logging, logging.handlers, StringIO

dir = os.path.dirname(__file__)
full_formatter = '%(asctime)s %(name)-12s %(levelname)-8s %(message)s'

try:
    LOG_FILENAME = '/var/log/hcs/python-hcs.log'
    open(LOG_FILENAME, 'a')
    # set up logging to file - see previous section for more details
    logging.basicConfig(level=logging.INFO,
                        format=full_formatter,
                        datefmt='%m-%d %H:%M',
                        filename=LOG_FILENAME,
                        filemode='a')
except IOError:
    # Might not actually need all of this, but do need some of it
    LOG_FILENAME = '/dev/null'
    open(LOG_FILENAME, 'a')
    # set up logging to file - see previous section for more details
    logging.basicConfig(level=logging.DEBUG,
                        format=full_formatter,
                        datefmt='%m-%d %H:%M',
                        filename=LOG_FILENAME,
                        filemode='a')

# define a Handler which writes INFO messages or higher to the sys.stderr
console = logging.StreamHandler()
console.setLevel(logging.INFO)
# set a format which is simpler for console use
formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
# tell the handler to use this format
console.setFormatter(formatter)
# add the handler to the root logger
logging.getLogger('').addHandler(console)

LOG_STRING = StringIO.StringIO()
log_string = logging.StreamHandler(LOG_STRING)
log_string.setLevel(logging.INFO)
log_string.setFormatter(logging.Formatter(full_formatter))
logging.getLogger('').addHandler(log_string)

def getlog():
    return LOG_STRING.getvalue()

def resetlog():
    global LOG_STRING, log_string
    logging.getLogger('').removeHandler(log_string)
    LOG_STRING = StringIO.StringIO()
    log_string = logging.StreamHandler(LOG_STRING)
    log_string.setLevel(logging.INFO)
    log_string.setFormatter(logging.Formatter(full_formatter))
    logging.getLogger('').addHandler(log_string)
