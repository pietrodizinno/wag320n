#! /bin/sh
# $Id: iptables_flush.sh,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $
IPTABLES=iptables

#flush all rules owned by miniupnpd
$IPTABLES -t nat -F MINIUPNPD
$IPTABLES -t filter -F MINIUPNPD

