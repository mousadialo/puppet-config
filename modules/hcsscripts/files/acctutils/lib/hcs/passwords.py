import os, re, yaml

_passwords_folder = '/etc/hcs/passwords'
_loaded = False
_loaded_data = {}

class PasswordError(AttributeError):
    pass

class PasswordNotFound(PasswordError):
    pass

def load():
    global _loaded_data, _loaded
    _loaded = True
    for file in os.listdir(_passwords_folder):
        if not re.search('\.yaml$', file):
            continue
        try:
            with open(os.path.join(_passwords_folder, file)) as f:
                data = yaml.load(f.read())
                _loaded_data.update(data)
        except IOError:
            # This will happen if you don't have perms
            pass

def get(name):
    if not _loaded:
        try:
            load()
        except OSError, e:
            msg = e.strerror
            if hasattr(e, 'filename'):
                msg += ': ' + e.filename
            raise PasswordNotFound(msg)
    password = _loaded_data
    split_name = name.split('.')
    try:
        for key in split_name:
            password = password[key]
        if isinstance(password, dict):
            # Default attr is password
            password = password['password']
    except (KeyError, TypeError):
        raise PasswordNotFound, 'Password %s not found!' % name
    if not isinstance(password, str):
        raise PasswordNotFound, 'Password %s not found!' % name
    return password
