/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file generate xml.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#ifndef __UPNPDESCGEN_H__
#define __UPNPDESCGEN_H__

/* for the root description 
 * The child list reference is stored in "data" member using the
 * INITHELPER macro with index/nchild always in the
 * same order, whatever the endianness */
struct XMLElt {
	const char * eltname;	/* begin with '/' if no child */
	const char * data;	/* Value */
};

char *genServiceDesc_xml(int *, char *);

void gen_root_xml(char *,char *,struct XMLElt *);
	
#endif

