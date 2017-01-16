#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "rc.h"
#include "nvram.h"

#define HOSTS_DOMAIN	"/etc/hosts"
#define RESOLVE_DOMAIN	"/etc/resolv.conf"

static int state;
int do_start(void)
{
	char *blank=nvram_safe_get("ca_blank_state");
	char *lan_ip=nvram_safe_get("lan_ipaddr");
	char *lan_if = nvram_safe_get("lan_if");
	FILE *fp=NULL;
	char tmp[128];
	char tmp2[128];
	char dns[512]="";

	fp=fopen(HOSTS_DOMAIN, "w");
	if(fp){
		fprintf(fp,"%s www.routerlogin.com\n",lan_ip);
		fprintf(fp,"%s routerlogin.com\n",lan_ip);
		fprintf(fp,"%s www.routerlogin.net\n",lan_ip);
		fprintf(fp,"%s routerlogin.net\n",lan_ip);
		fclose(fp);
	}
	if(access(RESOLVE_DOMAIN, F_OK) != 0)
		SYSTEM("/bin/cp %s %s", HOSTS_DOMAIN, RESOLVE_DOMAIN);
	if(*blank=='1')
		system("echo > /tmp/blank_state.out");
	else
		unlink("/tmp/blank_state.out");
	
	if(access("/tmp/wan_uptime",F_OK)!=0 && *blank=='1'){
		SYSTEM("iptables -t nat -D PREROUTING -i %s -p udp --dport 53 -j DNAT --to %s",
			lan_if, lan_ip);
		SYSTEM("iptables -t nat -I PREROUTING -i %s -p udp --dport 53 -j DNAT --to %s",
			lan_if, lan_ip);
		SYSTEM("dnrd -a %s -m hosts -c off -g routerlogin.com -b",lan_ip);
	}else{
		fp=fopen("/etc/resolv.conf","r");
		if(fp){
			while(fgets(tmp,128,fp)!=NULL && feof(fp)!=EOF){
				if(strstr(tmp,"nameserver")){
					sscanf(tmp,"nameserver %s",tmp2);
					sprintf(dns,"%s -s %s ",dns,tmp2);
				}
			}
			fclose(fp);
		}

		SYSTEM("dnrd -a %s -m hosts -c off --timeout=0 -b %s",lan_ip,dns);
		printf("dnrd -a %s -m hosts -c off --timeout=0 -b %s\n",lan_ip,dns);
	}
	return 0;
}
int do_stop(void)
{
	system("killall -9 dnrd");
	return 0;
}

int main(int argc, char **argv)
{
    int i;

    for (i = 0; i < argc; i ++) {
        sbup("argv[%d]: %s\n", i, argv[i]);
    }
    if (argc < 1) {
        sbup("usage: cmd start|stop\n");
        return 0;
    }
    state = get_state();
    sbup("now state: %c\n", state);
    
    if (Strcmp(*argv, "start")) {
        return do_start();
    } else if (Strcmp(*argv, "stop")) {
        return do_stop();
    } else if (Strcmp(*argv, "restart")) {
        do_stop();
        do_start();
    }
    sbup("set state to: %c\n", state);
    set_state(state);
    return 0;
}
