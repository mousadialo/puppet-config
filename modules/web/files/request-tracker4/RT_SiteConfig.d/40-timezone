# dynamically find out the current timezone
my $zone = "UTC";
$zone=`/bin/cat /etc/timezone`
    if -f "/etc/timezone";
chomp $zone;
Set($Timezone, $zone);
