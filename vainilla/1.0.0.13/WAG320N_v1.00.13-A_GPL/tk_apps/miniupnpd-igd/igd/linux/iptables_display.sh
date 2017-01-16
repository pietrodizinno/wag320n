#! /bin/sh
# $Id: iptables_display.sh,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $
IPTABLES=iptables

#display all chains relative to miniupnpd
$IPTABLES -v -n -t nat -L PREROUTING
$IPTABLES -v -n -t nat -L MINIUPNPD
$IPTABLES -v -n -t filter -L FORWARD
$IPTABLES -v -n -t filter -L MINIUPNPD

