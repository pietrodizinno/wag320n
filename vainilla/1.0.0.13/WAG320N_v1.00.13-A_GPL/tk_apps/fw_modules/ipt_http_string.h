
/* Kernel module to match a string into a packet.
 *
 * Copyright (C) 2000 Emmanuel Roger  <winfield@freegates.be>
 * 
 * ChangeLog
 *	19.02.2002: Gianni Tedesco <gianni@ecsc.co.uk>
 *		Fixed SMP re-entrancy problem using per-cpu data areas
 *		for the skip/shift tables.
 *	02.05.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 *		Fixed kernel panic, due to overrunning boyer moore string
 *		tables. Also slightly tweaked heuristic for deciding what
 * 		search algo to use.
 * 	27.01.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 * 		Implemented Boyer Moore Sublinear search algorithm
 * 		alongside the existing linear search based on memcmp().
 * 		Also a quick check to decide which method to use on a per
 * 		packet basis.
 * 		
 *  This software based on ipt_string.c by Emmanuel Roger <winfield@freegates.be>
 *
 *  Copyright (C) 2006-2009, SerComm Corporation
 *  All Rights Reserved.  
 * 		
 */

#ifndef _IPT_HTTP_STRING_H
#define _IPT_HTTP_STRING_H

#define MAX_URL_NUM 4
#define MAX_KEY_WORD_NUM 6

#define MAX_URL_LENGTH 64
#define MAX_KEY_WORD_LENGTH 64

struct ipt_http_string_info {
	char url_string[MAX_URL_NUM][MAX_URL_LENGTH + 1];
	int url_length[MAX_URL_NUM];
	char key_word[MAX_KEY_WORD_NUM][MAX_KEY_WORD_LENGTH + 1];
	int key_length[MAX_KEY_WORD_NUM];
	u_int16_t invert;
};

#endif /* _IPT_STRING_H */
