cd /usr/spool/log
rm syslog.7
mv syslog.6  syslog.7
mv syslog.5  syslog.6
mv syslog.4  syslog.5
mv syslog.3  syslog.4
mv syslog.2  syslog.3
mv syslog.1  syslog.2
mv syslog.0  syslog.1
mv syslog    syslog.0
cp /dev/null syslog
chmod 666    syslog
kill -1 `cat /etc/syslog.pid`
: @(#)newsyslog.sh 1.1 86/09/25 SMI
