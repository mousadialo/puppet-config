import driver, re, logging, custom_exceptions

logger = logging.getLogger('model')

class ModelMetaclass(driver.LDAPDriverMetaclass):
    """ Metaclass for model """

    def __getattr__(cls, attr):
        match = re.match('^find_by_(.*)', attr)
        if match:
            group = match.group(1)
            # Create the attribute so we don't have to do this
            # method_missing again
            setattr(cls, attr, cls._create_find_by(group))
            return getattr(cls, attr)
        else:
            raise AttributeError, 'No attribute "%s" found' % attr

class Model(driver.LDAPDriver):
    """ Model backend (based off of Rails's ActiveRecord). """

    __metaclass__ = ModelMetaclass

    observers = []
    tracking = []
    _callbacks = {}

    def __init__(self, **attrs):
#        self.frozen_attributes = {}
        self._state = {}
        for attr, value in attrs.iteritems():
            setattr(self, attr, value)
        self.new_record = True
        super(Model, self).__init__()
#        self.reset_tracking()

#     def __setattr__(self, attr):
#         self._record_change(attr)
#         super(Model, self).__setattr__(attr)

#     def freeze(self, attr):
#         self.frozen_attributes[attr] = True

#     def frozen(self, attr):
#         return attr in self.frozen_attributes

    def _require_assignment(self):
        raise AttributeError, 'Attribute has not yet been assigned'

    def _execute_callbacks(self, event):
        """
        Do callbacks, such as before_save.  If any callback returns a False (not None), stops the chain.
        """

        try:
            action = getattr(self, event)
        except AttributeError:
            pass
        else:
            if action() == False:
                return False
        
        try:
            self._callbacks[event]
        except KeyError:
            pass
        else:
            for callback in self._callbacks[event]:
                if callback(self) == False:
                    return False

        return self._invoke_observers(event)

    def update_attribute(self, attr, value):
        """ Update attribute and save value in backend.  Model must already be saved. """
        if self.new_record:
            raise custom_exceptions.ModelException, 'update_attribute called on unsaved model'
        setattr(self, attr, value)
        super(Model, self).save_attribute(attr)

    def _invoke_observers(self, event):
        """ Have observers watch event """
        for observer in self.observers:
                try:
                    action = getattr(observer, event)
                except AttributeError:
                    pass
                else:
                    if action(self) == False:
                        return False
        return True

    def save(self):
        if self.new_record:
            self.setstate('just-created', True)
        else:
            self.setstate('just-created', False)
        if self.state('just-created'):
            self._execute_callbacks('before_create')
        self._execute_callbacks('before_save')
        out = super(Model, self).save()
        if self.state('just-created'):
            self._execute_callbacks('after_create')
        self._execute_callbacks('after_save')
        return out

    @classmethod
    def add_observer(cls, observer):
        if not hasattr(cls, '_added_observer'):
            cls._added_observer = True
            cls.observers =  []
        cls.observers.append(observer)

    def ldap_equal(self, value1, value2):
        """ Checks if LDAP would find values equal """
        if isinstance(value1, str):
            value1 = [value1]
        if isinstance(value2, str):
            value1 = [value2]
        return value1 == value2

    def changed(self, attr):
        """ Hackish change detector, until I can make the real one work. """
        try:
            return not self.ldap_equal(getattr(self, 'original_%s' % attr), getattr(self, attr))
        except AttributeError:
            return not self.ldap_equal(getattr(self, 'default_%s' % attr)(), getattr(self, attr))
        
    def setstate(self, attr, value):
        self._state[attr] = value

    def state(self, attr):
        try:
            return self._state[attr]
        except KeyError:
            return None
        
#     def changed(self, attr):
#         try:
#             return self._original_attr_vals[attr] != getattr(self, attr)
#         except KeyError:
#             return False

#     def _record_change(self, attr):
#         if not self.changed(attr):
#             self._original_attr_vals[attr] = getattr(self, attr)
             
#     def reset_tracking(self, attr=None):
#         if attr:
#             del self._original_attr_vals[attr]
#         else:
#             self._original_attr_vals = {}

def stringField(*args, **opts):
    args = list(args)
    args.insert(0, 'string')
    return _create_property(*args, **opts)

def integerField(*args, **opts):
    args = list(args)
    args.insert(0, 'integer')
    return _create_property(*args, **opts)

def listField(*args, **opts):
    if isinstance(args, str):
        args = [args]
    else:
        args = list(args)
    args.insert(0, 'list')
    return _create_property(*args, **opts)
    
def aliasField(*args, **opts):
    args = list(args)
    args.insert(0, 'alias')
    return _create_property(*args, **opts)

num_properties = 0
def _create_property(*args, **opts):
    """ Create a property of a specific model """
    global num_properties
    type = args[0]

    # For aliases, just return the property you're aliasing
    if type == 'alias':
        return args[1]
    num_properties += 1
    attrvalue = '__%s_%d' % (type, num_properties)

    for arg in args[1:]:
        opts[arg] = True

    # Get the default value.  Maybe this is silly?
    default_val = opts.setdefault('default', None)
    postprocess = opts.setdefault('postprocess', None)
    null = opts.setdefault('null', True)
    if type == 'integer':
        postprocess = int
        if not null:
            default_val = 0
    elif type == 'string':
        postprocess = str
        if not null:
            default_val = ''
    elif type == 'list':
        def postprocess(val):
            if isinstance(val, list):
                return list(val)
            else:
                return [val]
        if not null:
            default_val = []
    else:
        raise 'Unrecognized type %s' % type

    def fget(self):
        import types
        try:
            val = getattr(self, attrvalue)
            if hasattr(val, '__call__'):
                val = val(self)
        except AttributeError:
            val = default_val
            if hasattr(val, '__call__'):
                val = val(self)
            if opts.setdefault('sticky_default', False):
                setattr(self, attrvalue, val)
        if postprocess:
            return val
        else:
            return postprocess(val)
     
    def fset(self, value):
        setattr(self, attrvalue, postprocess(value))

    newproperty = property(fget, fset)
    return newproperty

def attributesForCreation(*attrs):
    """ List of attributes needed to create a new object. """
    return attrs

def persistentAttributes(*attrs):
    """ List of attributes to be stored by the backend driver. """
    return attrs
