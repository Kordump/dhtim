#!/bin/bash
set -e
user=$1
chan=$2
pass=$3
host=$4
port=$5
pipe=.fifo-pipe
rm -f $pipe
mkfifo $pipe
cmd="./dhtpim -v -u $user -c $chan -p $pass -b $host -r $port"
echo "$cmd" > /dev/stderr
unbuffer cat $pipe | \
	( while true;do ( echo -e ":host NOTICE Auth :connected to dhtpim irc\n:host 001 $user :connect to dhtpim, 2\n:$user!$user@$user JOIN :#$chan" ; cat ) | unbuffer -p nc -l -p 6667 ; done ) | \
	unbuffer -p grep "^PRIVMSG #$chan :" | \
	unbuffer -p sed 's/^PRIVMSG [^ ]* :\([^\n]*\)\n*$/\1/'| \
	unbuffer -p grep -v '^$' | \
	( while true; do if ! $cmd ; then echo errored > /dev/stderr ; else echo quitted > /dev/stderr ; fi ; done ) | \
	unbuffer -p grep '^\[[0-9]*\] <[^>]*> ' | \
	unbuffer -p sed 's/^\[\([0-9]*\)\] <\([^>]*\)> \(.*\)$/:\2!\2@\2 PRIVMSG #'"$chan"' :\1 \3/' | \
	cat >> $pipe
