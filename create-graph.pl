#!/usr/bin/perl
# create daily graphs
use Time::Local;

$base  = "/mnt/nas/ekm";
$db    = "/root/prod.rrd";
$end   = time();
$start = $end - (24*60*60);

$dt = `date '+%Y%m%d'`;
$dt =~ s/\n//g;
$file = $dt . '_7638_Auden_Trl_utilities.png';

# TODO: Use RRDs
#print "start: $start end: $end\n";
$args  = "-s $start -e $end -w 1920 -h 1200 --full-size-mode  --x-grid MINUTE:10:HOUR:1:HOUR:2:0:\"%a %I:%M %p\"";
$out   = "$base/daily/$file";
$a0="DEF:watts=$db:watts:AVERAGE DEF:ccfw=$db:ccfw:AVERAGE DEF:ccfg=$db:ccfg:AVERAGE CDEF:wattsr=watts,0.1,\* CDEF:ccfwr=ccfw,748.051948,\* CDEF:ccfgr=ccfg,1000,\* AREA:wattsr#99de9c LINE2:wattsr#00AD07:\"Power (Watts/10)\" AREA:ccfgr#fbfe2899 LINE2:ccfgr#ffbb00:\"Gas (Cubic Feet)\" AREA:ccfwr#009cde99 LINE2:ccfwr#0007AD:\"Water (Gallons/10)\" ";
$cmd = "rrdtool graph $out $args $a0";
print "$cmd\n";
print `$cmd >/dev/null 2>&1`;

