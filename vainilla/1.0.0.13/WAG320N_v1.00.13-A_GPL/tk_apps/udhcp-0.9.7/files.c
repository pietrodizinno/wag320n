/*
 * files.c -- DHCP server file manipulation *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>

#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
#include "pidfile.h"
#include "socket.h"
/* on these functions, make sure you datatype matches */
static int read_ip(char *line, void *arg)
{
	struct in_addr *addr = arg;
	struct hostent *host;
	int retval = 1;

	if (!inet_aton(line, addr)) {
		if ((host = gethostbyname(line)))
			addr->s_addr = *((u_int32_t *) host->h_addr_list[0]);
		else retval = 0;
	}
	return retval;
}


static int read_str(char *line, void *arg)
{
	char **dest = arg;

	if (*dest) free(*dest);
	*dest = strdup(line);

	return 1;
}


static int read_u32(char *line, void *arg)
{
	u_int32_t *dest = arg;
	char *endptr;
	*dest = strtoul(line, &endptr, 0);
	return endptr[0] == '\0';
}


static int read_yn(char *line, void *arg)
{
	char *dest = arg;
	int retval = 1;

	if (!strcasecmp("yes", line))
		*dest = 1;
	else if (!strcasecmp("no", line))
		*dest = 0;
	else retval = 0;

	return retval;
}


/* read a dhcp option and add it to opt_list */
static int read_opt(char *line, void *arg)
{
	struct option_set **opt_list = arg;
	char *opt, *val, *endptr;
	struct dhcp_option *option = NULL;
	int retval = 0, length = 0;
	char buffer[255];
	u_int16_t result_u16;
	u_int32_t result_u32;
	int i;

	if (!(opt = strtok(line, " \t="))) return 0;

	for (i = 0; options[i].code; i++)
		if (!strcmp(options[i].name, opt))
			option = &(options[i]);

	if (!option) return 0;

	do {
		val = strtok(NULL, ", \t");
		if (val) {
			length = option_lengths[option->flags & TYPE_MASK];
			retval = 0;
			switch (option->flags & TYPE_MASK) {
			case OPTION_IP:
				retval = read_ip(val, buffer);
				break;
			case OPTION_IP_PAIR:
				retval = read_ip(val, buffer);
				if (!(val = strtok(NULL, ", \t/-"))) retval = 0;
				if (retval) retval = read_ip(val, buffer + 4);
				break;
			case OPTION_STRING:
				length = strlen(val);
				if (length > 0) {
					if (length > 254) length = 254;
					memcpy(buffer, val, length);
					retval = 1;
				}
				break;
			case OPTION_BOOLEAN:
				retval = read_yn(val, buffer);
				break;
			case OPTION_U8:
				buffer[0] = strtoul(val, &endptr, 0);
				retval = (endptr[0] == '\0');
				break;
			case OPTION_U16:
				result_u16 = htons(strtoul(val, &endptr, 0));
				memcpy(buffer, &result_u16, 2);
				retval = (endptr[0] == '\0');
				break;
			case OPTION_S16:
				result_u16 = htons(strtol(val, &endptr, 0));
				memcpy(buffer, &result_u16, 2);
				retval = (endptr[0] == '\0');
				break;
			case OPTION_U32:
				result_u32 = htonl(strtoul(val, &endptr, 0));
				memcpy(buffer, &result_u32, 4);
				retval = (endptr[0] == '\0');
				break;
			case OPTION_S32:
				result_u32 = htonl(strtol(val, &endptr, 0));
				memcpy(buffer, &result_u32, 4);
				retval = (endptr[0] == '\0');
				break;
			default:
				break;
			}
			if (retval)
				attach_option(opt_list, option, buffer, length);
		};
	} while (val && retval && option->flags & OPTION_LIST);
	return retval;
}

static struct config_kw_arr k_arr[MAX_INTERFACES][MAX_SERVERS_PER_IF] = 
{
	{
		{
			{/* keyword[14]			handler   	variable address						default[20] */
				{"server",			read_ip,  	&(server_config[0][0].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[0][0].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[0][0].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[0][0].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[0][0].options),			""},
				{"opt",				read_opt, 	&(server_config[0][0].options),			""},
				{"max_leases",		read_u32, 	&(server_config[0][0].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[0][0].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[0][0].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[0][0].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[0][0].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[0][0].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[0][0].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[0][0].lease_file),		"/var/udhcpd.leases"},
				{"pidfile",			read_str, 	&(server_config[0][0].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[0][0].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[0][0].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[0][0].sname),			""},
				{"boot_file",		read_str, 	&(server_config[0][0].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[0][0].vid),				"1"},
				{"enable", 			read_u32, 	&(server_config[0][0].enable), 			"1"}
			}
		},

		{
			{
				{"server",			read_ip,  	&(server_config[0][1].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[0][1].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[0][1].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[0][1].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[0][1].options),			""},
				{"opt",				read_opt, 	&(server_config[0][1].options),			""},
				{"max_leases",		read_u32, 	&(server_config[0][1].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[0][1].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[0][1].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[0][1].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[0][1].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[0][1].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[0][1].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[0][1].lease_file),		"/var/udhcpd01.leases"},
				{"pidfile",			read_str, 	&(server_config[0][1].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[0][1].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[0][1].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[0][1].sname),			""},
				{"boot_file",		read_str, 	&(server_config[0][1].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[0][1].vid),				"2"},
				{"enable", 			read_u32, 	&(server_config[0][1].enable), 			"1"}
				/*ADDME: static lease */
			}
		},

		{
			{
				{"server",			read_ip,  	&(server_config[0][2].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[0][2].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[0][2].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[0][2].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[0][2].options),			""},
				{"opt",				read_opt, 	&(server_config[0][2].options),			""},
				{"max_leases",		read_u32, 	&(server_config[0][2].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[0][2].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[0][2].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[0][2].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[0][2].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[0][2].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[0][2].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[0][2].lease_file),		"/var/udhcpd02.leases"},
				{"pidfile",			read_str, 	&(server_config[0][2].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[0][2].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[0][2].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[0][2].sname),			""},
				{"boot_file",		read_str, 	&(server_config[0][2].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[0][2].vid),				"3"},
				{"enable", 			read_u32, 	&(server_config[0][2].enable), 			"1"}
				/*ADDME: static lease */
			}
		},

		{
			{
				{"server",			read_ip,  	&(server_config[0][3].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[0][3].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[0][3].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[0][3].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[0][3].options),			""},
				{"opt",				read_opt, 	&(server_config[0][3].options),			""},
				{"max_leases",		read_u32, 	&(server_config[0][3].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[0][3].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[0][3].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[0][3].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[0][3].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[0][3].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[0][3].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[0][3].lease_file),		"/var/udhcpd03.leases"},
				{"pidfile",			read_str, 	&(server_config[0][3].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[0][3].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[0][3].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[0][3].sname),			""},
				{"boot_file",		read_str, 	&(server_config[0][3].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[0][3].vid),				"4"},
				{"enable", 			read_u32, 	&(server_config[0][3].enable), 			"1"}
				/*ADDME: static lease */
			}
		}

	},

	{
		{
			{/* keyword[14]			handler   	variable address						default[20] */
				{"server",			read_ip,  	&(server_config[1][0].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[1][0].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[1][0].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[1][0].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[1][0].options),			""},
				{"opt",				read_opt, 	&(server_config[1][0].options),			""},
				{"max_leases",		read_u32, 	&(server_config[1][0].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[1][0].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[1][0].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[1][0].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[1][0].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[1][0].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[1][0].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[1][0].lease_file),		"/var/udhcpd10.leases"},
				{"pidfile",			read_str, 	&(server_config[1][0].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[1][0].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[1][0].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[1][0].sname),			""},
				{"boot_file",		read_str, 	&(server_config[1][0].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[1][0].vid),				"1"},
				{"enable", 			read_u32, 	&(server_config[1][0].enable),			"1"}
			}
		},

		{
			{
				{"server",			read_ip,  	&(server_config[1][1].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[1][1].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[1][1].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[1][1].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[1][1].options),			""},
				{"opt",				read_opt, 	&(server_config[1][1].options),			""},
				{"max_leases",		read_u32, 	&(server_config[1][1].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[1][1].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[1][1].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[1][1].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[1][1].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[1][1].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[1][1].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[1][1].lease_file),		"/var/udhcpd11.leases"},
				{"pidfile",			read_str, 	&(server_config[1][1].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[1][1].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[1][1].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[1][1].sname),			""},
				{"boot_file",		read_str, 	&(server_config[1][1].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[1][1].vid),				"2"},
				{"enable", 			read_u32, 	&(server_config[1][1].enable), 			"1"}
				/*ADDME: static lease */
			}
		},

		{
			{
				{"server",			read_ip,  	&(server_config[1][2].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[1][2].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[1][2].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[1][2].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[1][2].options),			""},
				{"opt",				read_opt, 	&(server_config[1][2].options),			""},
				{"max_leases",		read_u32, 	&(server_config[1][2].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[1][2].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[1][2].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[1][2].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[1][2].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[1][2].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[1][2].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[1][2].lease_file),		"/var/udhcpd12.leases"},
				{"pidfile",			read_str, 	&(server_config[1][2].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[1][2].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[1][2].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[1][2].sname),			""},
				{"boot_file",		read_str, 	&(server_config[1][2].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[1][2].vid),				"3"},
				{"enable", 			read_u32, 	&(server_config[1][2].enable), 			"1"}
				/*ADDME: static lease */
			}
		},

		{
			{
				{"server",			read_ip,  	&(server_config[1][3].server),			"192.168.0.1"},
				{"start",			read_ip,  	&(server_config[1][3].start),			"192.168.0.2"},
				{"end",				read_ip,  	&(server_config[1][3].end),				"192.168.0.254"},
				{"interface",		read_str, 	&(server_config[1][3].interface),		LAN_INTERFACE},
				{"option",			read_opt, 	&(server_config[1][3].options),			""},
				{"opt",				read_opt, 	&(server_config[1][3].options),			""},
				{"max_leases",		read_u32, 	&(server_config[1][3].max_leases),		"254"},
				{"remaining",		read_yn,  	&(server_config[1][3].remaining),		"yes"},
				{"auto_time",		read_u32, 	&(server_config[1][3].auto_time),		"1"/*"7200"*/},
				{"decline_time",	read_u32, 	&(server_config[1][3].decline_time),	"3600"},
				{"conflict_time",	read_u32,	&(server_config[1][3].conflict_time),	"3600"},
				{"offer_time",		read_u32, 	&(server_config[1][3].offer_time),		"60"},
				{"min_lease",		read_u32, 	&(server_config[1][3].min_lease),		"60"},
				{"lease_file",		read_str, 	&(server_config[1][3].lease_file),		"/var/udhcpd13.leases"},
				{"pidfile",			read_str, 	&(server_config[1][3].pidfile),			"/var/run/udhcpd.pid"},
				{"notify_file", 	read_str, 	&(server_config[1][3].notify_file),		""},
				{"siaddr",			read_ip,  	&(server_config[1][3].siaddr),			"0.0.0.0"},
				{"sname",			read_str, 	&(server_config[1][3].sname),			""},
				{"boot_file",		read_str, 	&(server_config[1][3].boot_file),		""},
				{"vid",				read_u32, 	&(server_config[1][3].vid),				"4"},
				{"enable", 			read_u32, 	&(server_config[1][3].enable), 			"1"}
				/*ADDME: static lease */
			}
		}

	}
};

int read_config(char *file)
{
	FILE *in;
	char buffer[80], orig[80], *token, *line;
	int i, j;
	int index = 0; //index for interface
	int k = 0 ; // index for server of interface
	char chinterface[MAX_INTERFACES][80];
	char tmp[80];
	int  flag = 0;
	int interfaceid = -1;

	for(index = 0; index < MAX_INTERFACES; index++)
		no_of_servers[index] = 0;
	index = 0;
	for (index = 0; index < MAX_INTERFACES; index++)
		for(k = 0; k < MAX_SERVERS_PER_IF; k++)
			for (j = 0; j < MAX_KEYWORDS; j++)
				if (strlen(k_arr[index][k].keywords[j].def))
					k_arr[index][k].keywords[j].handler(k_arr[index][k].keywords[j].def, k_arr[index][k].keywords[j].var);

	if (!(in = fopen(file, "r"))) {
		LOG(LOG_ERR, "unable to open config file: %s", file);
		return 0;
	}
	index = 0;
	k = -1;

	while (fgets(buffer, 80, in))
	{
		if (strchr(buffer, '\n')) *(strchr(buffer, '\n')) = '\0';
		strncpy(orig, buffer, 80);
		if (strchr(buffer, '#')) *(strchr(buffer, '#')) = '\0';
		token = buffer + strspn(buffer, " \t");
		if (*token == '\0') continue;
		line = token + strcspn(token, " \t=");
		if (*line == '\0') continue;
		*line = '\0';
		line++;

		/* eat leading whitespace */
		line = line + strspn(line, " \t=");
		/* eat trailing whitespace */
		for (i = strlen(line) ; i > 0 && isspace(line[i-1]); i--);
		line[i] = '\0';

		if (!strcasecmp(token, "interface"))
		{
			memset(tmp, 0 , sizeof(tmp));
			//read_str(line, tmp);
			strcpy(tmp, line);
			if(flag == 0)
			{
				flag = 1;
				interfaceid++;
				strcpy(chinterface[interfaceid], tmp);
				no_of_servers[interfaceid]++;
			}
			else
			{
				if(strstr(tmp, chinterface[interfaceid]))
				{
					no_of_servers[interfaceid]++;
				}
				else
				{
					index++;
					k= -1;
					interfaceid++;
				}
				if(interfaceid >= MAX_INTERFACES)
					exit(-1);
			}

		}
		for (j = 0; j < MAX_KEYWORDS; j++)
		{
		//	if (!strcasecmp(token, "start")) {
			if (!strcasecmp(token, "server"))
			{
				k++;
			}

			if (!strcasecmp(token, k_arr[index][k].keywords[j].keyword))
			{
				if (!k_arr[index][k].keywords[j].handler(line, k_arr[index][k].keywords[j].var))
				{
					LOG(LOG_ERR, "unable to parse '%s'", orig);
					/* reset back to the default value */
					k_arr[index][k].keywords[j].handler(k_arr[index][k].keywords[j].def, k_arr[index][k].keywords[j].var);
				}

					break;
			}
		}
	}
	no_of_ifaces = interfaceid+1;
	LOG(LOG_INFO, "no_of_ifaces : %d no_of_servers[0]: %d no_of_servers[1]: %d", no_of_ifaces,
			no_of_servers[0], no_of_servers[1]);
	fclose(in);
	return 1;
}

void reconfig_dhcpd(int ifid)
{
	int i;
	int pid_fd;
	int j;
	struct option_set *option;

    	ifid = 0;


	/* ok, config file updated, we need commit the changing*/
	for (i = 0; i < MAX_INTERFACES; i++)
		for( j= 0; j < MAX_SERVERS_PER_IF; j++)
			memset(&server_config[i][j], 0, sizeof(struct server_config_t));

	read_config(DHCPD_CONF_FILE);

	if (no_of_ifaces == 0)
		return;

	for (i = 0; i < no_of_ifaces; i++)
		for( j= 0; j < no_of_servers[i]; j++)
	{
		pid_fd = pidfile_acquire(server_config[i][j].pidfile);
		pidfile_write_release(pid_fd);

		if ((option = find_option(server_config[i][j].options, DHCP_LEASE_TIME))) {
			memcpy(&server_config[i][j].lease, option->data + 2, 4);
			server_config[i][j].lease = ntohl(server_config[i][j].lease);
		}
		else server_config[i][j].lease = LEASE_TIME;

		//leases = malloc(sizeof(struct dhcpOfferedAddr) * server_config[i].max_leases);
		//memset(leases, 0, sizeof(struct dhcpOfferedAddr) * server_config[i].max_leases);

		//read_leases(server_config[i].lease_file, i);

		if (read_interface(server_config[i][j].interface, &server_config[i][j].ifindex,
			   &server_config[i][j].server, server_config[i][j].arp) < 0)
			server_config[i][j].active = FALSE;
		else
			server_config[i][j].active = TRUE;

#ifndef DEBUGGING
		pid_fd = pidfile_acquire(server_config[i][j].pidfile); /* hold lock during fork. */
		/* cfgmr req: do not fork */
		/*
		if (daemon(0, 0) == -1) {
			perror("fork");
			exit_server(1, i);
		}
		*/

		pidfile_write_release(pid_fd);
#endif
	}

	return;
}


/* write leases info into lease_file */
void write_leases(int ifid, int serverid)
{
	FILE *fp;
	unsigned int i;
	char buf[255];
	time_t curr = time(0);
	u_int32_t lease_time;
	
	if (!(fp = fopen(server_config[ifid][serverid].lease_file, "w"))) {
		LOG(LOG_ERR, "Unable to open %s for writing", server_config[ifid][serverid].lease_file);
		return;
	}
	for (i = 0; i < server_config[ifid][serverid].max_leases; i++) {
		
		if (leases[ifid][serverid][i].yiaddr != 0) {

		    if (server_config[ifid][serverid].remaining) {
			    if (lease_expired(&(leases[ifid][serverid][i])))
				    lease_time = 0;
			    else
			        lease_time = leases[ifid][serverid][i].expires - curr;
		    }
		    else
		        lease_time = leases[ifid][serverid][i].expires;
			lease_time = htonl(lease_time);
			fwrite(leases[ifid][serverid][i].chaddr, 16, 1, fp);
			fwrite(&(leases[ifid][serverid][i].yiaddr), 4, 1, fp);
			fwrite(&lease_time, 4, 1, fp);
			fwrite(leases[ifid][serverid][i].hostname, 256, 1, fp);
			fwrite(&leases[ifid][serverid][i].prop, 4, 1, fp);
			fwrite(&leases[ifid][serverid][i].if_flag, 1, 1, fp);
		}
	}
	fclose(fp);
	if (server_config[ifid][serverid].notify_file) {
		sprintf(buf, "%s %s", server_config[ifid][serverid].notify_file, server_config[ifid][serverid].lease_file);
		system(buf);
	}
}


void read_leases(char *file, int ifid, int serverid)
{
	FILE *fp;
	unsigned int i = 0;
	struct dhcpOfferedAddr lease;
	struct dhcpOfferedAddr *p;
	int ret=0;
	
	if (!(fp = fopen(file, "r"))) {
		LOG(LOG_ERR, "Unable to open %s for reading", file);
		return;
	}
	while (i < server_config[ifid][serverid].max_leases)
	{
		ret=fread(&lease, sizeof(lease), 1, fp);
		if(ret != 1){
			//LOG(LOG_INFO, "sizeof(lease): %d", sizeof(lease));
			//LOG(LOG_INFO, "ret: %d", ret);
			break;	
		}
		/* ADDME: is it a static lease */
		if (lease.yiaddr >= server_config[ifid][serverid].start && lease.yiaddr <= server_config[ifid][serverid].end)
		{
			lease.expires = ntohl(lease.expires);
			if (!server_config[ifid][serverid].remaining) lease.expires -= time(0);
#ifdef RONSCODE
			if (!(p = add_lease(lease.chaddr, lease.yiaddr, lease.expires, ifid, serverid, lease.hostname, lease.if_flag)))
			{
				LOG(LOG_WARNING, "Too many leases while loading %s\n", file);
				break;
			}

#else

			if (!(p = add_lease(lease.chaddr, lease.yiaddr, lease.expires, ifid, serverid)))
			{
				LOG(LOG_WARNING, "Too many leases while loading %s\n", file);
				break;
			}

#endif
            p->prop = lease.prop;
			i++;
		}
	}
	DEBUG(LOG_INFO, "Read %d leases", i);
	fclose(fp);
}


