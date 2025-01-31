/* addr2line.c -- convert addresses to line number and function name
   Copyright 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004
   Free Software Foundation, Inc.
   Contributed by Ulrich Lauther <Ulrich.Lauther@mchp.siemens.de>

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Derived from objdump.c and nm.c by Ulrich.Lauther@mchp.siemens.de

   Usage:
   addr2line [options] addr addr ...
   or
   addr2line [options]

   both forms write results to stdout, the second form reads addresses
   to be converted from stdin.  */

#include <string.h>

#include "bfd.h"
#include "getopt.h"
#include "libiberty.h"
#include "demangle.h"
#include "bucomm.h"
#include "budemang.h"

static bfd_boolean with_functions;	/* -f, show function names.  */
static bfd_boolean do_demangle;		/* -C, demangle names.  */
static bfd_boolean base_names;		/* -s, strip directory names.  */

static int naddr;		/* Number of addresses to process.  */
static char **addr;		/* Hex addresses to process.  */

static asymbol **syms;		/* Symbol table.  */

/* 150 isn't special; it's just an arbitrary non-ASCII char value.  */
#define OPTION_DEMANGLER	(150)

static struct option long_options[] =
{
  {"basenames", no_argument, NULL, 's'},
  {"demangle", optional_argument, NULL, 'C'},
  {"demangler", required_argument, NULL, OPTION_DEMANGLER},
  {"exe", required_argument, NULL, 'e'},
  {"functions", no_argument, NULL, 'f'},
  {"target", required_argument, NULL, 'b'},
  {"help", no_argument, NULL, 'H'},
  {"version", no_argument, NULL, 'V'},
  {0, no_argument, 0, 0}
};

static void usage (FILE *, int);
static void slurp_symtab (bfd *);
static void find_address_in_section (bfd *, asection *, void *);
static void translate_addresses (bfd *);
static void process_file (const char *, const char *);

/* Print a usage message to STREAM and exit with STATUS.  */

static void
usage (FILE *stream, int status)
{
  fprintf (stream, _("Usage: %s [option(s)] [addr(s)]\n"), program_name);
  fprintf (stream, _(" Convert addresses into line number/file name pairs.\n"));
  fprintf (stream, _(" If no addresses are specified on the command line, they will be read from stdin\n"));
  fprintf (stream, _(" The options are:\n\
  -b --target=<bfdname>  Set the binary file format\n\
  -e --exe=<executable>  Set the input file name (default is a.out)\n\
  -s --basenames         Strip directory names\n\
  -f --functions         Show function names\n\
  -C --demangle[=%s]\n\
                         Demangle function names\n\
     --demangler=<dso:function>\n\
                         Set dso and demangler function\n\
  -h --help              Display this information\n\
  -v --version           Display the program's version\n\
\n"), get_demangler_list ());

  list_supported_targets (program_name, stream);
  if (status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (status);
}

/* Read in the symbol table.  */

static void
slurp_symtab (bfd *abfd)
{
  long symcount;
  unsigned int size;

  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
    return;

  symcount = bfd_read_minisymbols (abfd, FALSE, (void *) &syms, &size);
  if (symcount == 0)
    symcount = bfd_read_minisymbols (abfd, TRUE /* dynamic */, (void *) &syms, &size);

  if (symcount < 0)
    bfd_fatal (bfd_get_filename (abfd));
}

/* These global variables are used to pass information between
   translate_addresses and find_address_in_section.  */

static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static bfd_boolean found;

/* Look for an address in a section.  This is called via
   bfd_map_over_sections.  */

static void
find_address_in_section (bfd *abfd, asection *section,
			 void *data ATTRIBUTE_UNUSED)
{
  bfd_vma vma;
  bfd_size_type size;

  if (found)
    return;

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
    return;

  vma = bfd_get_section_vma (abfd, section);
  if (pc < vma)
    return;

  size = bfd_get_section_size (section);
  if (pc >= vma + size)
    return;

  found = bfd_find_nearest_line (abfd, section, syms, pc - vma,
				 &filename, &functionname, &line);
}

/* Read hexadecimal addresses from stdin, translate into
   file_name:line_number and optionally function name.  */

static void
translate_addresses (bfd *abfd)
{
  int read_stdin = (naddr == 0);

  for (;;)
    {
      if (read_stdin)
	{
	  char addr_hex[100];

	  if (fgets (addr_hex, sizeof addr_hex, stdin) == NULL)
	    break;
	  pc = bfd_scan_vma (addr_hex, NULL, 16);
	}
      else
	{
	  if (naddr <= 0)
	    break;
	  --naddr;
	  pc = bfd_scan_vma (*addr++, NULL, 16);
	}

      found = FALSE;
      bfd_map_over_sections (abfd, find_address_in_section, NULL);

      if (! found)
	{
	  if (with_functions)
	    printf ("??\n");
	  printf ("??:0\n");
	}
      else
	{
	  if (with_functions)
	    {
	      const char *name;
	      char *alloc = NULL;

	      name = functionname;
	      if (name == NULL || *name == '\0')
		name = "??";
	      else if (do_demangle)
		{
		  alloc = demangle (abfd, name);
		  name = alloc;
		}

	      printf ("%s\n", name);

	      if (alloc != NULL)
		free (alloc);
	    }

	  if (base_names && filename != NULL)
	    {
	      char *h;

	      h = strrchr (filename, '/');
	      if (h != NULL)
		filename = h + 1;
	    }

	  printf ("%s:%u\n", filename ? filename : "??", line);
	}

      /* fflush() is essential for using this command as a server
         child process that reads addresses from a pipe and responds
         with line number information, processing one address at a
         time.  */
      fflush (stdout);
    }
}

/* Process a file.  */

static void
process_file (const char *file_name, const char *target)
{
  bfd *abfd;
  char **matching;

  if (get_file_size (file_name) < 1)
    return;

  abfd = bfd_openr (file_name, target);
  if (abfd == NULL)
    bfd_fatal (file_name);

  if (bfd_check_format (abfd, bfd_archive))
    fatal (_("%s: can not get addresses from archive"), file_name);

  if (! bfd_check_format_matches (abfd, bfd_object, &matching))
    {
      bfd_nonfatal (bfd_get_filename (abfd));
      if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
	{
	  list_matching_formats (matching);
	  free (matching);
	}
      xexit (1);
    }

  slurp_symtab (abfd);

  translate_addresses (abfd);

  if (syms != NULL)
    {
      free (syms);
      syms = NULL;
    }

  bfd_close (abfd);
}

int main (int, char **);

int
main (int argc, char **argv)
{
  const char *file_name;
  char *target;
  int c;
  const char *demangler = NULL, *style = NULL;

#if defined (HAVE_SETLOCALE) && defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
#if defined (HAVE_SETLOCALE)
  setlocale (LC_CTYPE, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = *argv;
  xmalloc_set_program_name (program_name);

  bfd_init ();
  set_default_bfd_target ();

  file_name = NULL;
  target = NULL;
  while ((c = getopt_long (argc, argv, "b:Ce:sfHhVv", long_options, (int *) 0))
	 != EOF)
    {
      switch (c)
	{
	case 0:
	  break;		/* We've been given a long option.  */
	case 'b':
	  target = optarg;
	  break;
	case 'C':
	  do_demangle = TRUE;
	  if (optarg != NULL)
	    style = optarg;
	  break;
	case 'e':
	  file_name = optarg;
	  break;
	case 's':
	  base_names = TRUE;
	  break;
	case 'f':
	  with_functions = TRUE;
	  break;
	case 'v':
	case 'V':
	  print_version ("addr2line");
	  break;
	case 'h':
	case 'H':
	  usage (stdout, 0);
	  break;
	case OPTION_DEMANGLER:
	  demangler = optarg;
	  break;
	default:
	  usage (stderr, 1);
	  break;
	}
    }

  init_demangler (style, NULL, demangler);

  if (file_name == NULL)
    file_name = "a.out";

  addr = argv + optind;
  naddr = argc - optind;

  process_file (file_name, target);

  return 0;
}
