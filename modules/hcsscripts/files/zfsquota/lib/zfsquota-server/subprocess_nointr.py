#!/usr/bin/env python
"""Process helper application"""

import errno
import os
import time
from subprocess import Popen, PIPE

def ignore_eintr(func):
    """Decorator: wrap a function in a loop that ignores OSError(EINTR)"""
    
    def newfunc(*args, **kwargs):
        while True:
            try:
                return func(*args, **kwargs)
            except OSError, e:
                if e.errno == errno.EINTR:
                    continue
                else:
                    raise
    
    return newfunc

@ignore_eintr
def waitpid_nointr(pid, opts):
    return os.waitpid(pid, opts)

def run(cmd, stdin=None):
    """
    Run given command with subprocess.Popen, ignoring signals during waitpid()
    
    Returns tuple of (returncode, stdout_text, stderr_text)
    
    run(cmd, stdin=None)
    @param cmd: Command and arguments
    @param stdin: String for standard input
    """
    
    p = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    
    # write input
    if stdin:
        p.stdin.write(stdin)
    p.stdin.close()
    
    # wait for it to finish, ignoring signals
    status = waitpid_nointr(p.pid, 0)[1]
    
    # get results
    returncode = os.WEXITSTATUS(status)
    stderr = ''.join(p.stderr.readlines())
    stdout = ''.join(p.stdout.readlines())
    
    return returncode, stdout, stderr

__all__=['run',]
