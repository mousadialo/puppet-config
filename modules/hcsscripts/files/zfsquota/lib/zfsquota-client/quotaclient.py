import math
import re
import xmlrpclib
import scylla.utils
import scylla.xmlrpc

_prefixes = [('P', 1 << 50),
             ('T', 1 << 40),
             ('G', 1 << 30),
             ('M', 1 << 20),
             ('K', 1 << 10),
             ('', 1)]

_prefix_dict = dict(_prefixes)

SERVER_HOST = 'https://hcs.harvard.edu'
SERVER_PORT = '8081'

def parse_num(string, unit='B'):
    """
    Parse a string as a number followed by an SI prefix and optional unit.
    """
    prefix_opts = '|'.join(_prefix_dict.keys())

    # match a numeric part followed by a unit part separated by any whitespace
    r = re.compile(r'^(-?\d+\.?\d*)\s*(%s)?%s?$' % (prefix_opts, unit))

    result = r.search(string)
    if not result:
        raise ValueError("Cannot parse "+string)

    numeric, prefix = result.groups()

    if '.' in numeric:
        number = float(numeric)
    else:
        number = int(numeric)

    return number * _prefix_dict[prefix]

def humanize(number, sig_figs=3, spacer=' ', unit='B'):
    """
    Give a number an appropriate base-2 prefix, making it human-readable.
    Significant figures half-implemented -- integer part is never truncated.
    """

    # find largest appropriate prefix
    for prefix, factor in _prefixes:
        if number >= factor:
            break

    # divide
    intpart, remainder = divmod(number, factor)
    fracpart = float(remainder) / factor

    # apply significant figures
    int_places = int(math.log10(intpart)) + 1 if intpart > 0 else 1
    decimal_places = int(sig_figs) - int_places

    if decimal_places > 0:
        fmt = '%.'+str(decimal_places)+'f'
        fracstr = fmt % fracpart
        if fracstr.startswith('1.'):
            intpart += 1
        return "%d.%s%s%s%s" % (intpart, fracstr[2:], spacer, prefix, unit)
    else:
        return '%i%s%s%s' % (intpart, spacer, prefix, unit)

def printValue(val):
    if val in ['-', 'none']:
        return "none"
    else:
        return humanize(int(val), 2)


class QuotaClient(object):
    def __init__(self, username, type):
        self.username = username
        self.type = type

        scylla.utils.VerifyAccountScyllaReady()
        t = scylla.xmlrpc.XmlRpcTransport()
        server_url = SERVER_HOST + ':' + SERVER_PORT + '/'
        self.server = xmlrpclib.ServerProxy(server_url, transport=t)

    def get_quota(self):
        """
        Get quota for given user from RPC server.
        Returns dict containing 'used', 'free', 'limit' (soft), 'quota' (hard).
        """

        d = self.server.get_quota(self.username, self.type)

        # convert strings back to ints (XML-RPC supports only 32-bit ints)
        for k, v in d.iteritems():
            try:
                d[k] = int(v)
            except ValueError:
                d[k] = v

        return d['used'], d['free'], d['limit'], d['quota']

    def get_all_quotas(self):
        """
        Get quota information for all available users from RPC server.

        NB: ZFS does not scale very well to large numbers of filesystems.
        This command can take a LONG time to complete, if it finishes at all.
        Because the server is currently synchronous, it will appear to be
        unresponsive until it finishes.
        """
        return self.server.get_all_quotas()

    def _set_softquota(self, bytes):
        """Set soft quota"""
        bytes = str(bytes)
        return self.server.set_softquota(self.username, self.type, bytes)

    def _set_hardquota(self, bytes):
        """Set hard quota"""
        bytes = str(bytes)
        return self.server.set_hardquota(self.username, self.type, bytes)

    def set_quota(self, hard=None, soft=None):
        """
        Set quota for given user.

        A value of None will leave the quota unchanged,
        any True value or 0 will set the quota (in bytes).
        """

        if hard is not None:
            self._set_hardquota(int(hard))

        if soft is not None:
            self._set_softquota(int(soft))


    def create_filesystem(self, uid, gid,
                          hardquota='', softquota=''):
        """
        Create a new zfs filesystem for the specified user.
        """

        # convert to str for transport (XML-RPC supports only 32-bit ints)
        if hardquota:
            hardquota = str(hardquota)
        if softquota:
            softquota = str(softquota)

        return self.server.create_filesystem(self.username, self.type, uid, gid,
                                             hardquota, softquota)

    def destroy_filesystem(self):
        """
        DESTROY a zfs filesystem. THERE IS NO UNDO.
        Will require the root user's SSL cert.
        """
        return self.server.destroy_filesystem(self.username, self.type)
