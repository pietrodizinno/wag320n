#ifndef _UTILS_H_
#define _UTILS_H_

//extern 
//extern
//extern
extern void poe_fatal (struct session *ses, char *fmt,...);
extern void poe_dbglog (struct session *ses ,char *fmt,...);
extern void poe_error (struct session *ses,char *fmt,...);
extern void poe_info (struct session *ses,char *fmt,...);
extern void poe_die (int status);



#endif /* _UTILS_H_ */

