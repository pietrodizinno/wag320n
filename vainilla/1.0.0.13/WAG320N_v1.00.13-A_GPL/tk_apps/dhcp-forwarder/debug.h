#ifndef _DEBUG_H_
#define _DEBUG_H_

#define ADA_DEBUG

#ifdef ADA_DEBUG
extern char *glb_dbg_file;
extern char *glb_dbg_func;
extern int glb_dbg_line;
#define debug   glb_dbg_file=__FILE__,glb_dbg_func=__FUNCTION__,glb_dbg_line=__LINE__,glb_debug
void glb_debug(char *format, ...);
#else
#define debug(format,...)
#endif				/* ADA_DEBUG */

#endif              /* _DEBUG_H_ */
