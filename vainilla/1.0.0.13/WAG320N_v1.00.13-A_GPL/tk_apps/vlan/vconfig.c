//
//Copyright (C) 2001  Ben Greear
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Library General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU Library General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// To contact the Author, Ben Greear:  greearb@candelatech.com
//


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include "boardparms.h"
#include <bcmnet.h>

#define MAX_HOSTNAME 256

static char* usage = 
      "\
Usage: enable          [interface-name] [vlan_id]\
       disable         [interface-name] [vlan_id]\
       query           [interface-name] [vlan_id]\
       add             [interface-name] [vlan_id]\
       rem             [vlan-name]\
       set_flag        [interface-name] [flag-num]       [0 | 1]\
       set_egress_map  [vlan-name]      [skb_priority]   [vlan_qos]\
       set_ingress_map [vlan-name]      [skb_priority]   [vlan_qos]\
       set_name_type   [name-type]\
\
* The [interface-name] is the name of the ethernet card that hosts\
  the VLAN you are talking about.\
* The vlan_id is the identifier (0-4095) of the VLAN you are operating on.\
* skb_priority is the priority in the socket buffer (sk_buff).\
* vlan_qos is the 3 bit priority in the VLAN header\
* name-type:  VLAN_PLUS_VID (vlan0005), VLAN_PLUS_VID_NO_PAD (vlan5),\
              DEV_PLUS_VID (eth0.0005), DEV_PLUS_VID_NO_PAD (eth0.5)\
* bind-type:  PER_DEVICE  # Allows vlan 5 on eth0 and eth1 to be unique.\
              PER_KERNEL  # Forces vlan 5 to be unique across all devices.\
* FLAGS:  1 REORDER_HDR  When this is set, the VLAN device will move the\
            ethernet header around to make it look exactly like a real\
            ethernet device.  This may help programs such as DHCPd which\
            read the raw ethernet packet and make assumptions about the\
            location of bytes.  If you don't need it, don't turn it on, because\
            there will be at least a small performance degradation.  Default\
            is OFF.\
";

void show_usage() {
   fprintf(stdout,usage);
}

int hex_to_bytes(char* bytes, int bytes_length, char* hex_str) {
   int hlen;
   int i;
   
   int j = 0;
   char hex[3];
   char* stop; /* not used for any real purpose */

   hlen = strlen(hex_str);

   hex[2] = 0;

   for (i = 0; i<hlen; i++) {

      hex[0] = hex_str[i];
      i++;
      if (i >= hlen) {
         return j; /* done */
      }
      
      hex[1] = hex_str[i];
      bytes[j++] = (char)strtoul(hex, &stop, 16);
   }
   return j;
}


#ifdef BUILD_STATIC
int vconfig_main(int argc, char** argv) {
#else
int main(int argc, char** argv) {
#endif
   int fd;
   struct vlan_ioctl_args if_request;
   
   char* cmd = NULL;
   char* if_name = NULL;
   unsigned int vid = 0;
   unsigned int skb_priority;
   unsigned short vlan_qos;
   unsigned int nm_type = VLAN_NAME_TYPE_PLUS_VID;
   ETHERNET_MAC_INFO EnetInfo;
   ETHERNET_MAC_INFO *pEnetInfo;

   char* conf_file_name = "/proc/net/vlan/config";

   memset(&if_request, 0, sizeof(struct vlan_ioctl_args));
   
   if ((argc < 3) || (argc > 5)) {
      fprintf(stdout,"Expecting argc to be 3-5, inclusive.  Was: %d\n",argc);

      show_usage();
      exit(1);
   }
   else {
      cmd = argv[1];
          
      if (strcasecmp(cmd, "set_name_type") == 0) {
         if (strcasecmp(argv[2], "VLAN_PLUS_VID") == 0) {
            nm_type = VLAN_NAME_TYPE_PLUS_VID;
         }
         else if (strcasecmp(argv[2], "VLAN_PLUS_VID_NO_PAD") == 0) {
            nm_type = VLAN_NAME_TYPE_PLUS_VID_NO_PAD;
         }
         else if (strcasecmp(argv[2], "DEV_PLUS_VID") == 0) {
            nm_type = VLAN_NAME_TYPE_RAW_PLUS_VID;
         }
         else if (strcasecmp(argv[2], "DEV_PLUS_VID_NO_PAD") == 0) {
            nm_type = VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD;
         }
         else {
            // MATHIEU
                    //cerr << "Invalid name type.\n";
            fprintf(stderr,"Invalid name type.\n");
                                 
            show_usage();
            exit(1);
         }
         if_request.u.name_type = nm_type;
      }
      else {
         if_name = argv[2];
         if (strlen(if_name) > 15) {
            // MATHIEU
                //cerr << "ERROR:  if_name must be 15 characters or less." << endl;
            fprintf(stderr,"ERROR:  if_name must be 15 characters or less.\n");
                        
                        exit(1);
         }
         strcpy(if_request.device1, if_name);
      }

      if (argc == 4) {
         vid = atoi(argv[3]);
         if_request.u.VID = vid;
      }

      if (argc == 5) {
         skb_priority = atoi(argv[3]);
         vlan_qos = atoi(argv[4]);
         if_request.u.skb_priority = skb_priority;
         if_request.vlan_qos = vlan_qos;
      }
   }

   // Open up the /proc/vlan/config
   if ((fd = open(conf_file_name, O_RDONLY)) < 0) {
      // MATHIEU
      //cerr << "ERROR:  Could not open /proc/vlan/config.\n";
      fprintf(stderr,"WARNING:  Could not open /proc/net/vlan/config. "
              "Maybe you need to load the 8021q module, or maybe you are "
              "not using PROCFS?? errno %d\n", errno);
          
   }
   else {
      close(fd);
   }

   /* We use sockets now, instead of the file descriptor */
   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      fprintf(stderr, "FATAL:  Couldn't open a socket..go figure!\n");
      exit(2);
   }   

   /* add */
   if ((strcasecmp(cmd, "enable") == 0) || (strcasecmp(cmd, "disable") == 0)) {
       int sockfd;
       struct ifreq intf;

       if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
           fprintf (stderr, "ERROR: Enabling VLAN %u to IF -:%s:- error: %s\n",
                    vid, if_name, strerror(errno));
           exit(2);
       }
       else {
           strncpy(intf.ifr_name, if_name, strlen(if_name));
           if (strcasecmp(cmd, "enable") == 0) {
               intf.ifr_data = (char *)&vid;
               if (ioctl(sockfd, SIOCGENABLEVLAN, &intf) < 0) {
                        fprintf(stderr,"ERROR: trying to enable VLAN #%u to IF -:%s:-  Try disable and"
                                " enable\n",
                                vid, if_name);
               }
           }
           else {
               if (ioctl(sockfd, SIOCGDISABLEVLAN, &intf) < 0) {
                        fprintf(stderr,"ERROR: trying to disable VLAN #%u to IF -:%s:-  error: %s\n",
                                vid, if_name, strerror(errno));
               }
           }
       }
       close(sockfd);
   }
   else if (strcasecmp(cmd, "query") == 0) {
       int sockfd;
       struct ifreq intf;

       if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
           fprintf (stderr, "ERROR: querying VLAN %u to IF -:%s:- error: %s\n",
                    vid, if_name, strerror(errno));
           exit(2);
       }
       else {
           strncpy(intf.ifr_name, if_name, strlen(if_name));
           intf.ifr_data = (char *)&EnetInfo;
           if (ioctl(sockfd, SIOCGQUERYNUMPORTS, &intf) < 0) {
                fprintf(stderr,"ERROR: trying to query VLAN #%u to IF -:%s:-\n",
                        vid, if_name);
           }
           else {
                pEnetInfo = (ETHERNET_MAC_INFO *)intf.ifr_data;
                fprintf(stderr,"%u\n", pEnetInfo->numSwitchPorts);
           }
       }
   }
   else if (strcasecmp(cmd, "add") == 0) {
      if_request.cmd = ADD_VLAN_CMD;
      if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
         fprintf(stderr,"ERROR: trying to add VLAN #%u to IF -:%s:-  error: %s\n",
                    vid, if_name, strerror(errno));                 
      }
      else {
         fprintf(stdout,"Added VLAN with VID == %u to IF -:%s:-\n",
                 vid, if_name);
         if (vid == 1) {
            fprintf(stdout, "WARNING:  VLAN 1 does not work with many switches,\nconsider another number if you have problems.\n");
         }
      }
   }//if
   else if (strcasecmp(cmd, "rem") == 0) {
      if_request.cmd = DEL_VLAN_CMD;
      if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
         fprintf(stderr,"ERROR: trying to remove VLAN -:%s:- error: %s\n",
                 if_name, strerror(errno));         
      }
      else {
         fprintf(stdout,"Removed VLAN -:%s:-\n", if_name);
      }
   }//if
   else if (strcasecmp(cmd, "set_egress_map") == 0) {
      if_request.cmd = SET_VLAN_EGRESS_PRIORITY_CMD;
      if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
         fprintf(stderr,"ERROR: trying to set egress map on device -:%s:- error: %s\n",
                 if_name, strerror(errno));         
      }
      else {
         fprintf(stdout,"Set egress mapping on device -:%s:- "
                 "Should be visible in /proc/net/vlan/%s\n",
                 if_name, if_name);
      }
   }
   else if (strcasecmp(cmd, "set_ingress_map") == 0) {
      if_request.cmd = SET_VLAN_INGRESS_PRIORITY_CMD;
      if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
         fprintf(stderr,"ERROR: trying to set ingress map on device -:%s:- error: %s\n",
                 if_name, strerror(errno));
      }
      else {
         fprintf(stdout,"Set ingress mapping on device -:%s:- "
                 "Should be visible in /proc/net/vlan/%s\n",
                 if_name, if_name);                
      }
   }   
   else if (strcasecmp(cmd, "set_flag") == 0) {
      if_request.cmd = SET_VLAN_FLAG_CMD;
      if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
         fprintf(stderr,"ERROR: trying to set flag on device -:%s:- error: %s\n",
                 if_name, strerror(errno));
      }
      else {
         fprintf(stdout,"Set flag on device -:%s:- "
                 "Should be visible in /proc/net/vlan/%s\n",
                 if_name, if_name);
      }
   }
   else if (strcasecmp(cmd, "set_name_type") == 0) {
      if_request.cmd = SET_VLAN_NAME_TYPE_CMD;
      if (ioctl(fd, SIOCSIFVLAN, &if_request) < 0) {
         fprintf(stderr,"ERROR: trying to set name type for VLAN subsystem, error: %s\n",
                 strerror(errno));         
      }
      else {
         fprintf(stdout,"Set name-type for VLAN subsystem."
                 " Should be visible in /proc/net/vlan/config\n");         
      }
   }
   else {
      fprintf(stderr, "Unknown command -:%s:-\n", cmd);

      show_usage();
      exit(5);
   }

   return 0;
}/* vconfig_main */

