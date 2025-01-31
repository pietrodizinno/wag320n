/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * David Hitz of Auspex Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

 /* 1999-02-22 Arkadiusz Mi�kiewicz <misiek@misiek.eu.org>
  * - added Native Language Support
  */

/*
 * look -- find lines in a sorted list.
 * 
 * The man page said that TABs and SPACEs participate in -d comparisons.
 * In fact, they were ignored.  This implements historic practice, not
 * the manual page.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <locale.h>
#include "pathnames.h"
#include "nls.h"

#define	EQUAL		0
#define	GREATER		1
#define	LESS		(-1)

int dflag, fflag;
/* uglified the source a bit with globals, so that we only need
   to allocate comparbuf once */
int stringlen;
char *string;
char *comparbuf;

static char *binary_search (char *, char *);
static int compare (char *, char *);
static void err (const char *fmt, ...);
static char *linear_search (char *, char *);
static int look (char *, char *);
static void print_from (char *, char *);
static void usage (void);

int
main(int argc, char *argv[])
{
	struct stat sb;
	int ch, fd, termchar;
	char *back, *file, *front, *p;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	
	setlocale(LC_ALL, "");

	file = _PATH_WORDS;
	termchar = '\0';
	string = NULL;		/* just for gcc */

	while ((ch = getopt(argc, argv, "adft:")) != EOF)
		switch(ch) {
		case 'a':
   		        file = _PATH_WORDS_ALT;
		        break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 't':
			termchar = *optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch (argc) {
	case 2:				/* Don't set -df for user. */
		string = *argv++;
		file = *argv;
		break;
	case 1:				/* But set -df by default. */
		dflag = fflag = 1;
		string = *argv;
		break;
	default:
		usage();
	}

	if (termchar != '\0' && (p = strchr(string, termchar)) != NULL)
		*++p = '\0';

	if ((fd = open(file, O_RDONLY, 0)) < 0 || fstat(fd, &sb))
		err("%s: %s", file, strerror(errno));
	if ((void *)(front = mmap(NULL,
				  (size_t)sb.st_size,
				  PROT_READ,
				  MAP_FILE|MAP_SHARED,
				  fd,
				  (off_t)0)) <= (void *)0)
		err("%s: %s", file, strerror(errno));
	back = front + sb.st_size;
	return look(front, back);
}

int
look(char *front, char *back)
{
	int ch;
	char *readp, *writep;

	/* Reformat string string to avoid doing it multiple times later. */
	if (dflag) {
		for (readp = writep = string; (ch = *readp++) != 0;) {
			if (isalnum(ch))
				*(writep++) = ch;
		}
		*writep = '\0';
		stringlen = writep - string;
	} else
		stringlen = strlen(string);

	comparbuf = malloc(stringlen+1);
	if (comparbuf == NULL)
		err(_("Out of memory"));

	front = binary_search(front, back);
	front = linear_search(front, back);

	if (front)
		print_from(front, back);
	return (front ? 0 : 1);
}


/*
 * Binary search for "string" in memory between "front" and "back".
 * 
 * This routine is expected to return a pointer to the start of a line at
 * *or before* the first word matching "string".  Relaxing the constraint
 * this way simplifies the algorithm.
 * 
 * Invariants:
 * 	front points to the beginning of a line at or before the first 
 *	matching string.
 * 
 * 	back points to the beginning of a line at or after the first 
 *	matching line.
 * 
 * Advancing the Invariants:
 * 
 * 	p = first newline after halfway point from front to back.
 * 
 * 	If the string at "p" is not greater than the string to match, 
 *	p is the new front.  Otherwise it is the new back.
 * 
 * Termination:
 * 
 * 	The definition of the routine allows it return at any point, 
 *	since front is always at or before the line to print.
 * 
 * 	In fact, it returns when the chosen "p" equals "back".  This 
 *	implies that there exists a string is least half as long as 
 *	(back - front), which in turn implies that a linear search will 
 *	be no more expensive than the cost of simply printing a string or two.
 * 
 * 	Trying to continue with binary search at this point would be 
 *	more trouble than it's worth.
 */
#define	SKIP_PAST_NEWLINE(p, back) \
	while (p < back && *p++ != '\n');

char *
binary_search(char *front, char *back)
{
	char *p;

	p = front + (back - front) / 2;
	SKIP_PAST_NEWLINE(p, back);

	/*
	 * If the file changes underneath us, make sure we don't
	 * infinitely loop.
	 */
	while (p < back && back > front) {
		if (compare(p, back) == GREATER)
			front = p;
		else
			back = p;
		p = front + (back - front) / 2;
		SKIP_PAST_NEWLINE(p, back);
	}
	return (front);
}

/*
 * Find the first line that starts with string, linearly searching from front
 * to back.
 * 
 * Return NULL for no such line.
 * 
 * This routine assumes:
 * 
 * 	o front points at the first character in a line. 
 *	o front is before or at the first line to be printed.
 */
char *
linear_search(char *front, char *back)
{
	while (front < back) {
		switch (compare(front, back)) {
		case EQUAL:		/* Found it. */
			return (front);
			break;
		case LESS:		/* No such string. */
			return (NULL);
			break;
		case GREATER:		/* Keep going. */
			break;
		}
		SKIP_PAST_NEWLINE(front, back);
	}
	return (NULL);
}

/*
 * Print as many lines as match string, starting at front.
 */
void 
print_from(char *front, char *back)
{
	int eol;

	while (front < back && compare(front, back) == EQUAL) {
		if (compare(front, back) == EQUAL) {
			eol = 0;
			while (front < back && !eol) {
				if (putchar(*front) == EOF)
					err("stdout: %s", strerror(errno));
				if (*front++ == '\n')
					eol = 1;
			}
		} else
			SKIP_PAST_NEWLINE(front, back);
	}
}

/*
 * Return LESS, GREATER, or EQUAL depending on how  string  compares with
 * string2 (s1 ??? s2).
 * 
 * 	o Matches up to len(s1) are EQUAL. 
 *	o Matches up to len(s2) are GREATER.
 * 
 * Compare understands about the -f and -d flags, and treats comparisons
 * appropriately.
 * 
 * The string "string" is null terminated.  The string "s2" is '\n' terminated
 * (or "s2end" terminated).
 *
 * We use strcasecmp etc, since it knows how to ignore case also
 * in other locales.
 */
int
compare(char *s2, char *s2end) {
	int i;
	char *p;

	/* copy, ignoring things that should be ignored */
	p = comparbuf;
	i = stringlen;
	while(s2 < s2end && *s2 != '\n' && i--) {
		if (!dflag || isalnum(*s2))
			*p++ = *s2;
		s2++;
	}
	*p = 0;

	/* and compare */
	if (fflag)
		i = strncasecmp(comparbuf, string, stringlen);
	else
		i = strncmp(comparbuf, string, stringlen);

	return ((i > 0) ? LESS : (i < 0) ? GREATER : EQUAL);
}

static void
usage()
{
	(void)fprintf(stderr, _("usage: look [-dfa] [-t char] string [file]\n"));
	exit(2);
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err(const char *fmt, ...)
#else
err(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "look: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(2);
	/* NOTREACHED */
}
