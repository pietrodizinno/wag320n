/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file generate device xml and service xml.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#ifndef __IGDDESCGEN_H__
#define __IGDDESCGEN_H__

/* char * genRootDesc(int *);
 * returns: NULL on error, string allocated on the heap */
char *
genRootDesc(int * len);

/* for the two following functions */
char *
genWANIPCn(int * len);

char *
genWANPPPCn(int * len);

char *
genWANCfg(int * len);

char *genLayer3F(int * len);

char *genWANEthLCfg(int * len);

char *genLANHCfgM(int * len);

extern struct  http_desc igd_http_desc[];

#endif

