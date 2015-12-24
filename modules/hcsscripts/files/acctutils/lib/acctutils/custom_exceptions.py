class CustomException(Exception):
    def __init__(self, value):
        self.parameter = value

    def __str__(self):
        return repr(self.parameter)    

class ObjectNotFound(CustomException):
    pass

class TooManyObjectsFound(CustomException):
    pass

class ModelException(CustomException):
    pass

class InvalidGroup(CustomException):
    pass
