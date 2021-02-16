#!/bin/sh
# 2014-11-20 Joe Hughes ekm@skydoo.net
# This script runs a program that gets RS-485 data from an EKM OmniMeter v4,
# saves the data to a csv file, and calls 'curl' to upload to wattvision.com

# This script is meant to be called from cron every minute, and should not
# generate any output other than what goes in the listed files.

OUT=/mnt/nas/ekm/$$.tmp
LOG=/mnt/nas/ekm/log.csv

#DT=`date "+%Y%m%d-%H%M%S"`
DT=`date +%s`

/root/bin/ekm > $OUT
RC=$?
if [ "$RC" -ne 0 ]; then
  sleep 1
  # try again
  /root/bin/ekm > $OUT
  RC=$?
  if [ "$RC" -ne 0 ]; then
    rm $OUT
    exit;
  fi
fi

KWH=`cat $OUT | awk '{print $1}'`
P1=`cat $OUT | awk '{print $2}'`
P2=`cat $OUT | awk '{print $3}'`
WATTS=`cat $OUT | awk '{print $4}'`
rm $OUT

echo $DT,$KWH,$P1,$P2,$WATTS>> $LOG

curl -d "{\"sensor_id\":\"15909802\",\"api_id\":\"vifpx9ty568qw7d4tqggf8bcgk06q56p\",\"api_key\":\"51pupxvatzj4r2u7fb4dmkpo46q2znlb\",\"watts\":\"$WATTS\",\"watthours\":\"$KWH\"}" http://www.wattvision.com/api/v0.2/elec >/dev/null 2>&1

#rrdtool update /mnt/nas/ekm/prod.rrd $DT:$WATTS
rrdtool update /mnt/nas/ekm/prod.rrd $DT:$KWH:$P1:$P2:$WATTS
