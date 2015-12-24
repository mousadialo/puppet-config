def camelcase_to_underscore(str):
    import re
    return re.sub('(((?<=[a-z])[A-Z])|([A-Z](?![A-Z]|$)))', '_\\1', str).lower().strip('_')

def randstring(length):
    import random,string
    str = string.letters + string.digits
    return ''.join([str[random.randint(0,len(str)-1)] for i in range(length)])

def word_wrap(string, width=80, ind1=0, ind2=0, prefix=''):
    """ word wrapping function.
        string: the string to wrap
        width: the column number to wrap at
        prefix: prefix each line with this string (goes before any indentation)
        ind1: number of characters to indent the first line
        ind2: number of characters to indent the rest of the lines
        Code from http://www.saltycrane.com/blog/2007/09/python-word-wrap-function/
    """
    string = prefix + ind1*" " + string
    newstring = ""
    if len(string) > width:
        while True:
            # find position of nearest whitespace char to the left of "width"
            marker = width-1
            while not string[marker].isspace():
                marker = marker - 1

            # remove line from original string and add it to the new string
            newline = string[0:marker] + "\n"
            newstring = newstring + newline
            string = prefix + ind2*" " + string[marker+1:]

            # break out of loop when finished
            if len(string) <= width:
                break
    
    return newstring + string

