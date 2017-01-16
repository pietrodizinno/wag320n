#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "debug.h"

#ifdef ADA_DEBUG

char *glb_dbg_file;
char *glb_dbg_func;
int glb_dbg_line;
void glb_debug(char *format, ...)
{
    va_list args;
    FILE *fp;

    fp = fopen("/tmp/debug", "a+");
    if (!fp) {
	    return;
    }
    
    fprintf(fp, "%s:%s():[%d]:", glb_dbg_file, glb_dbg_func,glb_dbg_line);

    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
}
#endif				/* ADA_DEBUG */
