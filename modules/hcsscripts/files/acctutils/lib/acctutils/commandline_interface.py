import sys, readline, os, pwd, atexit

myhome = pwd.getpwuid(os.getuid())[5]
readline_history_file = os.path.join(myhome, '.acctutils_history')

def save_history():
  readline.write_history_file(os.path.join(readline_history_file))
atexit.register(save_history)

try:
    readline.read_history_file(readline_history_file)
except IOError:
    # File DNE
    pass

class Requestor:
    password_attributes = ['password']

    @staticmethod
    def request(prompt, is_password=False, confirm=True, default=None):
        if is_password:
            while True:
                try:
                    input = Requestor.getpass(prompt, confirm)
                    break
                except ValueError, e:
                    print e
        else:
            input = raw_input('%s ' % prompt)
        if default is None or len(input) > 0:
            return input
        else:
            return default

    @staticmethod
    def getpass(prompt="Password: ", confirm=True):
        import getpass
        passwd = getpass.getpass(prompt) 
        if confirm:
            passwd2 = getpass.getpass("Repeat " + prompt)
            if passwd != passwd2:
                raise ValueError("Passwords do not match")
        return passwd


    @staticmethod
    def request_opt(message, **args):
        args['lower'] = True
        try:
            if args['default']:
                message = '%s [Yn]' % message
                args['default'] = 'y'
            else:
                message = '%s [yN]' % message
                args['default'] = 'n'
        except KeyError:
            pass
        opt = Requestor.request_attribute('opt', '%s ' % message, **args)
        if opt == 'y':
            return True
        else:
            return False

    @staticmethod
    def request_attribute(attr, message, lower=False, default=None):
        if Requestor.password_attributes.__contains__(attr):
            confirm = is_password = True
        else:
            confirm = is_password = False
        check_attribute = getattr(Requestor, 'check_%s' % attr, False)
        while True:
            input = Requestor.request(message, is_password, confirm)
            if lower: input = input.lower()
            if default and len(input) == 0: input = default
            if check_attribute:
                error = check_attribute(input)
                if error is None: break
                else: print error
            else:
                break
        return input
    
    @staticmethod
    def check_opt(opt):
        if opt.lower() in ['y', 'n']:
            return None
        else:
            return "Must choose y or n"
