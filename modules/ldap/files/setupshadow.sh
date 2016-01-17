#!/bin/sh
# Configures certain shadow attributes to be self-writable so that 
# Linux can update shadowLastChange when a user updates his/her password

if [ "$1" ] ; then
    ldapport=$1
else
    ldapport=389
fi

if [ -z "$DMPWD" ] ; then
    echo "when prompted, provide the directory manager password"
    echo -n "Password:"
    stty -echo
    read dmpwd
    stty echo
else
    dmpwd="$DMPWD"
fi

ldapmodify -x -H ldap://localhost:$ldapport -D "cn=Directory Manager" -w "$dmpwd" <<EOF
dn: ou=People,dc=hcs,dc=harvard,dc=edu
changetype: modify
add: aci
aci: (targetattr="userPassword||shadowLastChange||shadowMin||shadowMax||shadowWarning||shadowInactive||shadowExpire||shadowFlag")(version 3.0; acl "Enable self write for shadow attributes"; allow (write)userdn = "ldap:///self";)

EOF
