"""
Module for retrieving info about HCS.  Uses YAML data store.

As an example, suppose that the data store is the following:

office_hours:
    day: Wednesday
    start: '7:30'
    end: '10:00'
    room_number: SOCH 307
    office_name: Hilles office

The room number would then be accessible via
info.office_hours.room_number, for example.  The info.office_hours
object is an instantiation of the OfficeHours class.
"""
import os, re, yaml

def humanize(name):
    return ''.join([piece.capitalize() for piece in name.split('_')])

def setup():
    for class_name, attrs in _loaded_data.iteritems():
        cls = globals()[humanize(class_name)]
        globals()[class_name] = cls()
        for key, value in attrs.iteritems():
            setattr(cls, key, value)

_info_file = '/etc/hcs/info.yaml'
_loaded_data = yaml.load(open(_info_file).read())

class OfficeHours(object):
    def __str__(self):
        return "%(day)ss from %(start)s-%(end)s PM, in our %(office_name)s, %(room_number)s" % OfficeHours.__dict__

if not __name__ == '__main__':
    setup()
