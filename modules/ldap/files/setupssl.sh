#!/bin/sh
# Adapted from https://github.com/richm/scripts/blob/master/setupssl2.sh
# Tutorial for setting up SSL/TLS: http://directory.fedoraproject.org/docs/389ds/howto/howto-ssl.html

if [ "$1" -a -d "$1" ] ; then
    secdir="$1"
    echo "Using $1 as sec directory"
else
    secdir=/etc/dirsrv/slapd-localhost
fi

if [ "$2" ] ; then
    ldapport=$2
else
    ldapport=389
fi

if [ "$3" ] ; then
    ldapsport=$3
else
    ldapsport=636
fi

me=`whoami`
if [ "$me" = "root" ] ; then
    isroot=1
fi

# see if there are already certs and keys
if [ -f $secdir/cert8.db ] ; then
    # look for CA cert
    if certutil -L -d $secdir -n "CA certificate" 2> /dev/null ; then
        echo "Using existing CA certificate"
    else
        echo "No CA certificate found - will create new one"
        needCA=1
    fi

    # look for server cert
    if certutil -L -d $secdir -n "Server-Cert" 2> /dev/null ; then
        echo "Using existing directory Server-Cert"
    else
        echo "No Server Cert found - will create new one"
        needServerCert=1
    fi

    prefix="new-"
    prefixarg="-P $prefix"
else
    needCA=1
    needServerCert=1
fi

# get our user and group
if test -n "$isroot" ; then
    uid=`/bin/ls -ald $secdir | awk '{print $3}'`
    gid=`/bin/ls -ald $secdir | awk '{print $4}'`
fi

# 2. Create a password file for your security token password:
if [ -n "$needCA" -o -n "$needServerCert" ] ; then
    if [ -f $secdir/pwdfile.txt ] ; then
        echo "Using existing $secdir/pwdfile.txt"
    else
        echo "Creating password file for security token"
        (ps -ef ; w ) | sha1sum | awk '{print $1}' > $secdir/pwdfile.txt
        if test -n "$isroot" ; then
            chown $uid:$gid $secdir/pwdfile.txt
        fi
        chmod 400 $secdir/pwdfile.txt
    fi

# 3. Create a "noise" file for your encryption mechanism: 
    if [ -f $secdir/noise.txt ] ; then
        echo "Using existing $secdir/noise.txt file"
    else
        echo "Creating noise file"
        (w ; ps -ef ; date ) | sha1sum | awk '{print $1}' > $secdir/noise.txt
        if test -n "$isroot" ; then
            chown $uid:$gid $secdir/noise.txt
        fi
        chmod 400 $secdir/noise.txt
    fi

# 4. Create the key3.db and cert8.db databases:
    if [ -z "$prefix" ] ; then
        echo "Creating initial key and cert db"
    else
        echo "Creating new key and cert db"
    fi
    certutil -N $prefixarg -d $secdir -f $secdir/pwdfile.txt
    if test -n "$isroot" ; then
        chown $uid:$gid $secdir/${prefix}key3.db $secdir/${prefix}cert8.db
    fi
    chmod 600 $secdir/${prefix}key3.db $secdir/${prefix}cert8.db
fi

getserialno() {
    SERIALNOFILE=${SERIALNOFILE:-$secdir/serialno.txt}
    if [ ! -f $SERIALNOFILE ] ; then
        echo ${BEGINSERIALNO:-1000} > $SERIALNOFILE
    fi
    serialno=`cat $SERIALNOFILE`
    expr $serialno + 1 > $SERIALNOFILE
    echo $serialno
}

if test -n "$needCA" ; then
# 5. Generate the encryption key:
    echo "Creating encryption key for CA"
    certutil -G $prefixarg -d $secdir -z $secdir/noise.txt -f $secdir/pwdfile.txt
# 6. Generate the self-signed certificate: 
    echo "Creating self-signed CA certificate"
# note - the basic constraints flag (-2) is required to generate a real CA cert
# it asks 3 questions that cannot be supplied on the command line
    serialno=`getserialno`
    ( echo y ; echo ; echo y ) | certutil -S $prefixarg -n "CA certificate" -s "cn=CAcert" -x -t "CT,," -m $serialno -v 120 -d $secdir -z $secdir/noise.txt -f $secdir/pwdfile.txt -2
# export the CA cert for use with other apps
    echo Exporting the CA certificate to cacert.asc
    certutil -L $prefixarg -d $secdir -n "CA certificate" -a > $secdir/cacert.asc
fi

if test -n "$MYHOST" ; then
    myhost="$MYHOST"
else
    myhost=`hostname --fqdn`
fi

genservercert() {
    hostname=${1:-`hostname --fqdn`}
    certname=${2:-"Server-Cert"}
    serialno=${3:-`getserialno`}
    ou=${OU:-"389 Directory Server"}
    certutil -S $prefixarg -n "$certname" -s "cn=$hostname,ou=$ou" -c "CA certificate" -t "u,u,u" -m $serialno -v 120 -d $secdir -z $secdir/noise.txt -f $secdir/pwdfile.txt
}

remotehost() {
    # the subdir called $host will contain all of the security files to copy to the remote system
    mkdir -p $secdir/$1
    # this is stupid - what we want is that each key/cert db for the remote host has a
    # cert with nickname "Server-Cert" - however, badness:
    # 1) pk12util cannot change nick either during import or export
    # 2) certutil does not have a way to change or rename the nickname
    # 3) certutil cannot create two certs with the same nick
    # so we have to copy all of the secdir files to the new server specific secdir
    # and create everything with copies
    cp -p $secdir/noise.txt $secdir/pwdfile.txt $secdir/cert8.db $secdir/key3.db $secdir/secmod.db $secdir/$1
    SERIALNOFILE=$secdir/serialno.txt secdir=$secdir/$1 genservercert $1
}

if [ -n "$REMOTE" ] ; then
    for host in $myhost ; do
        remotehost $host
    done
elif test -n "$needServerCert" ; then
# 7. Generate the server certificate:
    for host in $myhost ; do
        echo Generating server certificate for 389 Directory Server on host $host
        echo Using fully qualified hostname $host for the server name in the server cert subject DN
        echo Note: If you do not want to use this hostname, export MYHOST="host1 host2 ..." $0 ...
        genservercert $host
    done
fi

# create the pin file
if [ ! -f $secdir/pin.txt ] ; then
    echo Creating pin file for directory server
    pinfile=$secdir/pin.txt
    echo 'Internal (Software) Token:'`cat $secdir/pwdfile.txt` > $pinfile
    if test -n "$isroot" ; then
        chown $uid:$gid $pinfile
    fi
    chmod 400 $pinfile
else
    echo Using existing $secdir/pin.txt
fi

if [ -n "$REMOTE" ] ; then
    for host in $myhost ; do
        cp -p $secdir/pin.txt $secdir/$host
    done
fi

if [ -n "$needCA" -o -n "$needServerCert" ] ; then
    if [ -n "$prefix" ] ; then
    # move the old files out of the way
        mv $secdir/cert8.db $secdir/orig-cert8.db
        mv $secdir/key3.db $secdir/orig-key3.db
    # move in the new files - will be used after server restart
        mv $secdir/${prefix}cert8.db $secdir/cert8.db
        mv $secdir/${prefix}key3.db $secdir/key3.db
    fi
fi

# enable SSL in the directory server
echo "Enabling SSL in the directory server"
if [ -z "$DMPWD" ] ; then
    echo "when prompted, provide the directory manager password"
    echo -n "Password:"
    stty -echo
    read dmpwd
    stty echo
else
    dmpwd="$DMPWD"
fi

ldapmodify -x -h localhost -p $ldapport -D "cn=directory manager" -w "$dmpwd" <<EOF
dn: cn=encryption,cn=config
changetype: modify
replace: nsSSLClientAuth
nsSSLClientAuth: allowed
-
add: nsSSL3Ciphers
nsSSL3Ciphers: +all

dn: cn=config
changetype: modify
add: nsslapd-security
nsslapd-security: on
-
replace: nsslapd-ssl-check-hostname
nsslapd-ssl-check-hostname: off
-
replace: nsslapd-secureport
nsslapd-secureport: $ldapsport

dn: cn=RSA,cn=encryption,cn=config
changetype: add
objectclass: top
objectclass: nsEncryptionModule
cn: RSA
nsSSLPersonalitySSL: Server-Cert
nsSSLToken: internal (software)
nsSSLActivation: on

EOF

echo "Done.  You must restart the directory server and the admin server for the changes to take effect."
