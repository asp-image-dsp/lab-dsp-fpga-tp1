/*
	NAME
		cofdmp - Motorola DSP COFF file dump utility

	SYNOPSIS
		cofdmp [-cfhloqrstv] [-d <file>] files

	DESCRIPTION

		This program reads and displays Motorola DSP COFF object files.

	OPTIONS
			-c	Dump string table.
			-d	Dump to output file.
			-f	Dump file header.
			-h	Dump section headers.
			-l	Dump line number information.
			-o	Dump optional header.
			-q	Do not display signon banner.
			-r	Dump relocation information.
			-s	Dump section contents.
			-t	Dump symbol table.
			-v	Dump symbolically.
			-x	Dump in test mode.

	NOTES
		Adapted from Gintaras R. Gircys, "Understanding and Using
		COFF," O'Reilly & Associates, 1988.  Similar to the System
		V coffdump utility.

	DIAGNOSTICS
		Error messages are sent to the standard error output when
		files cannot be open.

	HISTORY
		1.0	The beginning.
		1.1	Added public domain getopt() code.
		1.2	Removed default output file argument;
			added explicit -o option.
			Revamped error() routine.
		1.3	Added error checking on fseek(), fread()
			calls and dereferences of string table.
			Put in code to display link header.
		1.4	Added byte swap logic for little-endian hosts.
		1.5	Added test mode for regression testing.
		See CVS log for subsequent history.
*/


#if !defined (LINT)
static char rcsid[] = "$Id: cofdmp.c,v 1.57 1998/01/07 19:39:51 jay Exp $";	/* RCS data */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#if !defined (ICC) && !defined (WATCOM)
extern int errno;
#endif
#if !defined (VARARGS)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#if defined (VAXC) || defined (MPW) || defined (ATARI)
#include <types.h>
#else
#include <sys/types.h>
#endif
#if defined (MPW)
#include <files.h>
#include <CursorCtl.h>
#endif
#if defined (LINT) || defined (SUN)
#include "lint.h"	/* lint declarations */
#endif

/*
  Headers for working with COFF files
*/

#ifdef BIG_ENDIAN
#if !BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#endif
#include "coreaddr.h"
#include "maout.h"
#include "dspext.h"

#include "cofdmp.h"

/*
  Global variables
*/

char Progdef[] = "cofdmp";		/* program default name */
char *Progname = Progdef;		/* pointer to program name */
char Progtitle[] = "Motorola DSP COFF File Dump Utility";
/*
  Put an extra space after the version number so that it is not
  compressed in the object file.  That way the strings program
  can find it.
*/
char Version[]   = "Version 6.2 ";	/* cofdmp version number */
char Copyright[] = "(C) Copyright Motorola, Inc. 1991-1996.  All rights reserved.";

char *optarg = NULL;	/* command argument pointer */
int optind = 0;		/* command argument index */

FILHDR	file_header;	/* File header structure */
AOUTHDR	opt_header;	/* Optional header structure */
OPTHDR2	link_header;	/* Linker header structure */
int	absolute;	/* Absolute file flag */
int	test = NO;	/* Test mode flag */
int	post4_1 = NO;	/* Source debug flag */
int	sdi = NO;	/* Span-dependent instruction flag */

long num_sections;	/* Number of sections */
long section_seek;	/* Used to seek to first section */

long symptr;		/* File pointer to symbol table entries */
long num_symbols;	/* Number of symbols */

char *str_tab;		/* Pointer to start of string char. array */
long str_length;	/* Length in bytes of string array */

FILE *ifile = NULL;	/* file pointer for input file */
char *ifn = NULL;	/* pointer to input file name */
FILE *ofile = NULL;	/* file pointer for output file */
char *ofn = NULL;	/* pointer to output file name */

char string_flag = YES;		/* dump strings flag */
char filhdr_flag = YES;		/* dump file header flag */
char sechdr_flag = YES;		/* dump section headers flag */
char lineno_flag = YES;		/* dump line numbers flag */
char opthdr_flag = YES;		/* dump optional header flag */
char reloc_flag = YES;		/* dump relocation info flag */
char data_flag = YES;		/* dump section data flag */
char symbol_flag = YES;		/* dump symbol flag */
char verbose = NO;		/* dump symbolically */
char quiet = NO;		/* do not display signon banner */
char isarray = NO;		/* global array flag */

char indent[] = "    ";		/* indentation string */

int	Mon_lengths[2][MONS_PER_YEAR] = {	/* count of days in months */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

int	Year_lengths[2] = {	/* count of days in (leap) year */
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

char *memstr[] = {		/* memory mapping string array */
"P:",
"X:","Y:","L:","",
"LAA:","LAB:","LBA:","LBB:","LE:",
"LI:","PA:","PB:","PE:","PI:",
"PR:","XA:","XB:","XE:","XI:",
"XR:","YA:","YB:","YE:","YI:",
"YR:","PT:","PF:","EMI:",
"E0:","E1:","E2:","E3:",
"E4:","E5:","E6:","E7:",
"E8:","E9:","E10:","E11:",
"E12:","E13:","E14:","E15:",
"E16:","E17:","E18:","E19:",
"E20:","E21:","E22:","E23:",
"E24:","E25:","E26:","E27:",
"E28:","E29:","E30:","E31:",
"E32:","E33:","E34:","E35:",
"E36:","E37:","E38:","E39:",
"E40:","E41:","E42:","E43:",
"E44:","E45:","E46:","E47:",
"E48:","E49:","E50:","E51:",
"E52:","E53:","E54:","E55:",
"E56:","E57:","E58:","E59:",
"E60:","E61:","E62:","E63:",
"E64:","E65:","E66:","E67:",
"E68:","E69:","E70:","E71:",
"E72:","E73:","E74:","E75:",
"E76:","E77:","E78:","E79:",
"E80:","E81:","E82:","E83:",
"E84:","E85:","E86:","E87:",
"E88:","E89:","E90:","E91:",
"E92:","E93:","E94:","E95:",
"E96:","E97:","E98:","E99:",
"E100:","E101:","E102:","E103:",
"E104:","E105:","E106:","E107:",
"E108:","E109:","E110:","E111:",
"E112:","E113:","E114:","E115:",
"E116:","E117:","E118:","E119:",
"E120:","E121:","E122:","E123:",
"E124:","E125:","E126:","E127:",
"E128:","E129:","E130:","E131:",
"E132:","E133:","E134:","E135:",
"E136:","E137:","E138:","E139:",
"E140:","E141:","E142:","E143:",
"E144:","E145:","E146:","E147:",
"E148:","E149:","E150:","E151:",
"E152:","E153:","E154:","E155:",
"E156:","E157:","E158:","E159:",
"E160:","E161:","E162:","E163:",
"E164:","E165:","E166:","E167:",
"E168:","E169:","E170:","E171:",
"E172:","E173:","E174:","E175:",
"E176:","E177:","E178:","E179:",
"E180:","E181:","E182:","E183:",
"E184:","E185:","E186:","E187:",
"E188:","E189:","E190:","E191:",
"E192:","E193:","E194:","E195:",
"E196:","E197:","E198:","E199:",
"E200:","E201:","E202:","E203:",
"E204:","E205:","E206:","E207:",
"E208:","E209:","E210:","E211:",
"E212:","E213:","E214:","E215:",
"E216:","E217:","E218:","E219:",
"E220:","E221:","E222:","E223:",
"E224:","E225:","E226:","E227:",
"E228:","E229:","E230:","E231:",
"E232:","E233:","E234:","E235:",
"E236:","E237:","E238:","E239:",
"E240:","E241:","E242:","E243:",
"E244:","E245:","E246:","E247:",
"E248:","E249:","E250:","E251:",
"E252:","E253:","E254:","E255:",
"D:","P8:","U:","U8:","U16:"
};

int memstrlen = sizeof (memstr) / sizeof (char *);

char *typstr[] = {		/* data type string pointer array */
"T_NULL",
"T_ARG",
"T_CHAR",
"T_SHORT",
"T_INT",
"T_LONG",
"T_FLOAT",
"T_DOUBLE",
"T_STRUCT",
"T_UNION",
"T_ENUM",
"T_MOE",
"T_UCHAR",
"T_USHORT",
"T_UINT",
"T_ULONG",
"T_FRAC",
"T_UFRAC",
"T_LFRAC",
"T_ULFRAC",
"T_ACCUM",
"T_LACCUM"
};

static int typstrlen = sizeof (typstr) / sizeof (char *);

char *ftypstr[] = {		/* file type string pointer array */
"T_NULL",
"T_MOD"
};

static int ftypstrlen = sizeof (ftypstr) / sizeof (char *);

#ifdef  MSIL_EXTENSIONS
static SYMENT *symtab;
struct sym_idx {
    int index;
    int section;
    int offset;
};

struct sym_idx *sorted_symbol_index;
int n_sorted_symbols;

#endif /* MSIL_EXTENSIONS */

int 
main (int argc, char *argv[])
{
	int c;			/* option character buffer */
	int cleared = NO;	/* option flags cleared */

/*
	set up for signals, save program name, check for command line options
*/

#if defined (MPW)
	InitCursorCtl (NULL);
#endif
	(void)signal (SIGINT, onsig);   /* set up for signals */
	(void)signal (SIGSEGV, onsig);  /* set up for fatal signals */

	/* scan for quiet flag on command line */
	quiet = cmdarg ('q', argc, argv) ? YES : NO;

#if defined (MSDOS) || defined (TOS)
	if (!quiet)
		(void)fprintf (stderr, "%s  %s\n%s\n",
			       Progtitle, Version, Copyright);
#endif
	while ((c = getopts (argc, argv, "CcD:d:FfHhLlOoQqRrSsTtVvXx")) != EOF) {
		/* clear all option flags if we got anything other than
		   just a modifier (e.g. -d, -q, -v, -x)
		 */
		if (!cleared && strchr ("DdQqVvXx", c) == NULL) {
			clear_opts ();
			cleared = YES;
		}
		if (isupper (c))
			c = tolower (c);
		switch (c) {
		case 'c':
			string_flag = YES;
			break;
		case 'd':
			ofn = optarg;
			break;
		case 'f':
			filhdr_flag = YES;
			break;
		case 'h':
			sechdr_flag = YES;
			break;
		case 'l':
			lineno_flag = YES;
			break;
		case 'o':
			opthdr_flag = YES;
			break;
		case 'q':
			quiet = YES;
			break;
		case 'r':
			reloc_flag = YES;
			break;
		case 's':
			data_flag = YES;
			break;
		case 't':
			symbol_flag = YES;
			break;
		case 'v':
			verbose = YES;
			break;
		case 'x':
			test = YES;
			break;
		case '?':
		default:
			usage ();
			break;
		}
	}

	/* no output file specified? use stdout */
	if (!ofn)
		ofile = stdout;
	else {
		if (!(ofile = fopen (ofn, "w")))
			error ("cannot open output file %s", ofn);
		(void) setfile (ofn, "TEXT", "MPS ");
	}

	/* no more args?  error */
	if (optind >= argc)
		usage ();

	/* dash for input filename? use stdin */
	if (*argv[optind] == '-') {
		ifn = "stdin";
		ifile = stdin;
		dump_file ();
	} else while (optind < argc) {	/* process input files */
		ifn = argv[optind++];
		if (!(ifile = fopen (ifn, "rb")))
			error ("cannot open input file %s", ifn);
		dump_file ();
		(void)fclose (ifile);
	}
	exit (0);
	/*NOTREACHED*/	/* for lint, Watcom */
	return (0);
}


static void 
clear_opts (void)
{
	string_flag =
	filhdr_flag =
	sechdr_flag =
	lineno_flag =
	opthdr_flag =
	reloc_flag =
	data_flag =
	symbol_flag = NO;
}

static void 
dump_file (void)
{
	read_headers ();
	read_strings ();
#ifdef  MSIL_EXTENSIONS
	if (verbose)
		read_symbols();
#endif /* MSIL_EXTENSIONS */
	if (filhdr_flag)
		dump_filhdr ();
	if (opthdr_flag)
		dump_opthdr ();
	if (sechdr_flag || reloc_flag || lineno_flag || data_flag)
		read_sections ();
	if (symbol_flag)
		dump_symbols ();
	if (string_flag)
		dump_strings ();
}

static void 
read_headers (void)
{
	if (freads ((char *)&file_header, sizeof (FILHDR), 1, ifile) != 1)
		error ("cannot read file header");
	if (!ISCOFF (file_header.f_magic))
		error ("invalid object file format");

	/* Save the global values */

	num_sections = file_header.f_nscns;
	num_symbols = file_header.f_nsyms;
	symptr = file_header.f_symptr;
	absolute = !!(file_header.f_flags & F_RELFLG);
	post4_1 = file_header.f_opthdr != OPTHSZ;
	sdi = !!(file_header.f_flags & F_SDI);

	if (file_header.f_opthdr) {	/* optional header present */
		if (absolute) {
			if (freads ((char *)&opt_header,
				   (int)file_header.f_opthdr, 1, ifile) != 1)
				error ("cannot read optional file header");
		} else {
			if (freads ((char *)&link_header,
				   (int)file_header.f_opthdr, 1, ifile) != 1)
				error ("cannot read linker file header");
		}
	}

	/* File offset for first section headers */
	section_seek = FILHZ + file_header.f_opthdr;
}

static void 
read_strings (void)
{
	long	strings;
	if (num_symbols==0) return;
	
	strings = symptr + (num_symbols * SYMESZ);
	if (fseek (ifile, strings, 0) != 0)
		error ("cannot seek to string table length");
	if (freads ((char *)&str_length, 4, 1, ifile) != 1 && !feof (ifile))
		error ("cannot read string table length");
	if (feof (ifile))
		str_length = 0L;
	else if (str_length) {
		str_length -= 4;
		if (str_length < 0)
			error ("invalid string table length");
		str_tab = (char *)malloc ((unsigned)str_length);
		if (fseek (ifile, strings + 4, 0) != 0)
			error ("cannot seek to string table");
		if (fread (str_tab, (int)str_length, 1, ifile) != 1)
			error ("cannot read string table");
	}
}

static void 
dump_filhdr (void)
{
	eprintf (ofile, "\nFILE HEADER");
	if (test)
		eprintf (ofile, "\n");
	else
		eprintf (ofile, " FOR FILE %s\n", ifn);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "f_magic  = 0%lo", file_header.f_magic);
	if (!verbose)
		eprintf (ofile, "\n");
	else {
		char *machtype = NULL;
		switch (file_header.f_magic) {
		case M56KMAGIC:
			machtype = "DSP56000";
			break;
		case M96KMAGIC:
			machtype = "DSP96000";
			break;
		case M16KMAGIC:
			machtype = "DSP56100";
			break;
		case M563MAGIC:
			machtype = "DSP56300";
			break;
		case M568MAGIC:
			machtype = "DSP56800";
			break;
		case M566MAGIC:
			machtype = "DSP56600";
			break;                        
		case M569MAGIC:
			machtype = "DSP56900";
			break;                        
		default:
			break;
		}
		if (!machtype)
			eprintf (ofile, "\n");
		else
			eprintf (ofile, " [%s]\n", machtype);
	}
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "f_nscns  = %lu\n", file_header.f_nscns);
	if (!verbose || test) {
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "f_timdat = 0x%08lX\n",
			       file_header.f_timdat);
	} else {
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "f_timdat = %s",
			       asctime (dectime ((time_t *)&file_header.f_timdat)));
	}
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "f_symptr = %ld\n", file_header.f_symptr);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "f_nsyms  = %ld\n", file_header.f_nsyms);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "f_opthdr = %lu\n", file_header.f_opthdr);
	if (!verbose) {
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "f_flags  = 0%lo (0x%lX)\n",
			       file_header.f_flags, file_header.f_flags);
	} else {
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "f_flags  = ");
		if (!file_header.f_flags)
			eprintf (ofile, "NONE\n");
		else {
			if (file_header.f_flags & F_RELFLG)
				eprintf (ofile, "F_RELFLG ");
			if (file_header.f_flags & F_EXEC)
				eprintf (ofile, "F_EXEC ");
			if (file_header.f_flags & F_LNNO)
				eprintf (ofile, "F_LNNO ");
			if (file_header.f_flags & F_LSYMS)
				eprintf (ofile, "F_LSYMS ");
			if (file_header.f_flags & F_MINMAL)
				eprintf (ofile, "F_MINMAL ");
			if (file_header.f_flags & F_UPDATE)
				eprintf (ofile, "F_UPDATE ");
			if (file_header.f_flags & F_CC)
				eprintf (ofile, "F_CC ");
			if (file_header.f_flags & F_SDI)
				eprintf (ofile, "F_SDI ");
			eprintf (ofile, "\n");
		}
	}
}

static void 
dump_opthdr (void)
{
	if (file_header.f_opthdr) {	/* optional header present */

		if (absolute) {

			eprintf (ofile, "\nOPTIONAL HEADER");
			if (test)
				eprintf (ofile, "\n");
			else
				eprintf (ofile, " FOR FILE %s\n", ifn);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "magic      = 0%lo\n",
				       opt_header.magic);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "vstamp     = %ld\n",
				       opt_header.vstamp);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "tsize      = %ld\n",
				       opt_header.tsize);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "dsize      = %ld\n",
				       opt_header.dsize);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "bsize      = %ld\n",
				       opt_header.bsize);
			eprintf (ofile, "%s", indent);
			if (!verbose ||
			    CORE_ADDR_MAP (opt_header.entry) < 0 ||
			    CORE_ADDR_MAP (opt_header.entry) >= memstrlen ||
			    CORE_ADDR_MAP (opt_header.text_start) < 0 ||
			    CORE_ADDR_MAP (opt_header.text_start) >= memstrlen ||
			    CORE_ADDR_MAP (opt_header.data_start) < 0 ||
			    CORE_ADDR_MAP (opt_header.data_start) >= memstrlen ||
			    CORE_ADDR_MAP (opt_header.text_end) < 0 ||
			    CORE_ADDR_MAP (opt_header.text_end) >= memstrlen ||
			    CORE_ADDR_MAP (opt_header.data_end) < 0 ||
			    CORE_ADDR_MAP (opt_header.data_end) >= memstrlen) {
				eprintf (ofile,
					 "entry      = 0x%08lX 0x%08lX\n",
					 CORE_ADDR_MAP (opt_header.entry),
					 CORE_ADDR_ADDR (opt_header.entry));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "text_start = 0x%08lX 0x%08lX\n",
					 CORE_ADDR_MAP (opt_header.text_start),
					 CORE_ADDR_ADDR (opt_header.text_start));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "data_start = 0x%08lX 0x%08lX\n",
					 CORE_ADDR_MAP (opt_header.data_start),
					 CORE_ADDR_ADDR (opt_header.data_start));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "text_end   = 0x%08lX 0x%08lX\n",
					 CORE_ADDR_MAP (opt_header.text_end),
					 CORE_ADDR_ADDR (opt_header.text_end));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "data_end   = 0x%08lX 0x%08lX\n",
					 CORE_ADDR_MAP (opt_header.data_end),
					 CORE_ADDR_ADDR (opt_header.data_end));
			} else {
				eprintf (ofile,
					 "entry      = %s0x%08lX\n",
					 memstr[CORE_ADDR_MAP (opt_header.entry)],
					 CORE_ADDR_ADDR (opt_header.entry));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "text_start = %s0x%08lX\n",
					 memstr[CORE_ADDR_MAP (opt_header.text_start)],
					 CORE_ADDR_ADDR (opt_header.text_start));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "data_start = %s0x%08lX\n",
					 memstr[CORE_ADDR_MAP (opt_header.data_start)],
					 CORE_ADDR_ADDR (opt_header.data_start));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "text_end   = %s0x%08lX\n",
					 memstr[CORE_ADDR_MAP (opt_header.text_end)],
					 CORE_ADDR_ADDR (opt_header.text_end));
				eprintf (ofile, "%s", indent);
				eprintf (ofile,
					 "data_end   = %s0x%08lX\n",
					 memstr[CORE_ADDR_MAP (opt_header.data_end)],
					 CORE_ADDR_ADDR (opt_header.data_end));
			}
		} else {

			eprintf (ofile, "\nLINKER HEADER");
			if (test)
				eprintf (ofile, "\n");
			else
				eprintf (ofile, " FOR FILE %s\n", ifn);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "modsize    = %ld\n",
				       link_header.modsize);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "datasize   = %ld\n",
				       link_header.datasize);
			if (!verbose) {
				eprintf (ofile, "%s", indent);
				eprintf (ofile, "endstr     = %ld\n",
					       link_header.endstr);
			} else {
				if (link_header.endstr < 0L) {
					eprintf (ofile, "%s", indent);
					eprintf (ofile, "endstr     = NONE\n");
				} else {
					if (link_header.endstr < (long)sizeof (str_length) ||
					    link_header.endstr > str_length)
						error ("invalid string table offset for end expression string");
					eprintf (ofile, "%s", indent);
					eprintf (ofile, "endstr     = %s\n",
						       &str_tab[link_header.endstr - sizeof (str_length)]);
				}
			}
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "secnt      = %ld\n",
				       link_header.secnt);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "ctrcnt     = %ld\n",
				       link_header.ctrcnt);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "relocnt    = %ld\n",
				       link_header.relocnt);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "lnocnt     = %ld\n",
				       link_header.lnocnt);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "bufcnt     = %ld\n",
				       link_header.bufcnt);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "ovlcnt     = %ld\n",
				       link_header.ovlcnt);
			if (post4_1) {	/* not old object format */
				eprintf (ofile, "%s", indent);
				eprintf (ofile, "majver     = %ld\n",
					 test ? 0L : link_header.majver);
				eprintf (ofile, "%s", indent);
				eprintf (ofile, "minver     = %ld\n",
					 test ? 0L : link_header.minver);
				eprintf (ofile, "%s", indent);
				eprintf (ofile, "revno      = %ld\n",
					 test ? 0L : link_header.revno);
			}
			if (sdi) {
				eprintf (ofile, "%s", indent);
				eprintf (ofile, "sditot     = %ld\n",
					 link_header.sditot);
			}
		}
	}
}

static void 
read_sections (void)
{
	int i, sn;
	XCNHDR	sh;		/* Section header structure */

	for (i = 0; i < num_sections; i++) {
#if defined (MPW)
		SpinCursor (4);
#endif
		if (fseek (ifile, section_seek, 0) != 0)
			error ("cannot seek to section headers");
		if (freads ((char *)&sh, sizeof (XCNHDR), 1, ifile) != 1)
			error ("cannot read section headers");
#if !defined (BIG_ENDIAN)
		if (sh._n._s_n._s_zeroes)
			swapw (sh._n._s_name, sizeof (long), 2);
#endif
		section_seek += sizeof (XCNHDR);
		sn = i + 1;
		if (sechdr_flag)
			dump_sh (&sh, sn);
		if (data_flag)
			dump_data (&sh, sn);
		if (reloc_flag)
			dump_re (&sh, sn);
		if (lineno_flag)
			dump_le (&sh, sn);
	}
}


static char *
get_secname (XCNHDR *sh)
{
	char	*secname;

	if (sh->_n._s_n._s_zeroes)
		secname = sh->_n._s_name;
	else {
		if (sh->_n._s_n._s_offset < (long)sizeof (str_length) ||
		    sh->_n._s_n._s_offset > str_length)
			error ("invalid string table offset for section header name");
		secname = &str_tab[sh->_n._s_n._s_offset - sizeof (str_length)];
	}
	return (secname);
}

static void
dump_sh (XCNHDR *sh, int sn)
{
	eprintf (ofile, "\nSECTION HEADER FOR SECTION %s (%d)",
		       get_secname (sh), sn);
	if (test)
		eprintf (ofile, "\n");
	else
		eprintf (ofile, " IN FILE %s\n", ifn);
	eprintf (ofile, "%s", indent);
	if (!verbose ||
	    CORE_ADDR_MAP (sh->_s.s_paddr) < 0 ||
	    CORE_ADDR_MAP (sh->_s.s_paddr) >= memstrlen ||
	    CORE_ADDR_MAP (sh->_s.s_vaddr) < 0 ||
	    CORE_ADDR_MAP (sh->_s.s_vaddr) >= memstrlen) {
		eprintf (ofile, "s_paddr   = 0x%08lX 0x%08lX\n",
			 CORE_ADDR_MAP (sh->_s.s_paddr),
			 CORE_ADDR_ADDR (sh->_s.s_paddr));
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "s_vaddr   = 0x%08lX 0x%08lX\n",
			 CORE_ADDR_MAP (sh->_s.s_vaddr),
			 CORE_ADDR_ADDR (sh->_s.s_vaddr));
	} else {
		eprintf (ofile, "s_paddr   = %s0x%08lX\n",
			 memstr[CORE_ADDR_MAP (sh->_s.s_paddr)],
			 CORE_ADDR_ADDR (sh->_s.s_paddr));
		eprintf (ofile, "%s", indent);
		if (sh->_s.s_flags & STYP_BLOCK)
			eprintf (ofile, "s_vaddr   = %ld\n",
				 CORE_ADDR_ADDR (sh->_s.s_vaddr));
		else
			eprintf (ofile, "s_vaddr   = %s0x%08lX\n",
				 memstr[CORE_ADDR_MAP (sh->_s.s_vaddr)],
				 CORE_ADDR_ADDR (sh->_s.s_vaddr));
	}
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "s_size    = %ld\n", sh->_s.s_size);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "s_scnptr  = %ld\n", sh->_s.s_scnptr);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "s_relptr  = %ld\n", sh->_s.s_relptr);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "s_lnnoptr = %ld\n", sh->_s.s_lnnoptr);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "s_nreloc  = %lu\n", sh->_s.s_nreloc);
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "s_nlnno   = %lu\n", sh->_s.s_nlnno);
	if (!verbose) {
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "s_flags   = 0%lo (0x%lX)\n",
			       sh->_s.s_flags, sh->_s.s_flags);
	} else {
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "s_flags   = ");
		if (sh->_s.s_flags & STYP_DSECT)
			eprintf (ofile, "STYP_DSECT ");
		else if (sh->_s.s_flags & STYP_NOLOAD)
			eprintf (ofile, "STYP_NOLOAD ");
		else if (sh->_s.s_flags & STYP_GROUP)
			eprintf (ofile, "STYP_GROUP ");
		else if (sh->_s.s_flags & STYP_PAD)
			eprintf (ofile, "STYP_PAD ");
		else if (sh->_s.s_flags & STYP_COPY)
			eprintf (ofile, "STYP_COPY ");
		else {
			eprintf (ofile, "STYP_REG ");
			if (sh->_s.s_flags & STYP_BSS)
				eprintf (ofile, "STYP_BSS ");
			else if (sh->_s.s_flags & STYP_TEXT)
				eprintf (ofile, "STYP_TEXT ");
			else if (sh->_s.s_flags & STYP_DATA)
				eprintf (ofile, "STYP_DATA ");
			if (sh->_s.s_flags & STYP_BLOCK)
				eprintf (ofile, "STYP_BLOCK ");
			if (sh->_s.s_flags & STYP_OVERLAY)
				eprintf (ofile, "STYP_OVERLAY ");
			if (sh->_s.s_flags & STYP_OVERLAYP)
				eprintf (ofile, "STYP_OVERLAYP ");
			if (sh->_s.s_flags & STYP_MACRO)
				eprintf (ofile, "STYP_MACRO ");
			if (sh->_s.s_flags & STYP_BW)
				eprintf (ofile, "STYP_BW ");
		}
		eprintf (ofile, "\n");
	}
}

static void
dump_data (XCNHDR *sh, int sn)
{
	char	*secname;
	long	*raw_data;
	int	j;

	if (sh->_s.s_scnptr && sh->_s.s_size) {
		secname = get_secname (sh);
		eprintf (ofile, "\nRAW DATA FOR SECTION %s (%d)",
			       secname, sn);
		if (test)
			eprintf (ofile, "\n");
		else
			eprintf (ofile, " IN FILE %s\n", ifn);
		if (sh->_s.s_size < 0)
			error ("invalid raw data size in section %s",
			       secname);
		raw_data = (long *)malloc
			((unsigned)(sh->_s.s_size * sizeof (long)));
		if (fseek (ifile, sh->_s.s_scnptr, 0) != 0)
			error ("cannot seek to raw data in section %s",
			       secname);
		if (freads ((char *)raw_data, (int)sh->_s.s_size,
			   sizeof (long), ifile) != sizeof (long))
			error ("cannot read raw data in section %s",
			       secname);
		j = 0;
		eprintf (ofile, "%s", indent);
#ifdef  MSIL_EXTENSIONS
		/*
		 * Invoke the disassembly routine.
		 */
		if (verbose && sh->_s.s_flags & STYP_TEXT)
			disassemble_text(sh, sn, raw_data);
		else
#endif /* MSIL_EXTENSIONS */
		while (j < sh->_s.s_size) {
			eprintf (ofile, "%08lX ", *raw_data);
			raw_data++;
			j++;
			if (j % RAW == 0 && j < sh->_s.s_size)
				eprintf (ofile, "\n%s", indent);
		}
#ifdef MSIL_EXTENSIONS
		if (!(verbose && sh->_s.s_flags & STYP_TEXT))
#endif /* MSIL_EXTENSIONS */
		eprintf (ofile, "\n");
		free ((char *)(raw_data - j));
	}
}

static void
dump_re (XCNHDR *sh, int sn)
{
	char	*secname;
	RELOC	re;		/* Relocation entry structure */
	int	j;

	if (sh->_s.s_relptr && sh->_s.s_nreloc) {
		secname = get_secname (sh);
		eprintf (ofile, "\nRELOCATION ENTRIES FOR SECTION %s (%d)", secname, sn);
		if (test)
			eprintf (ofile, "\n");
		else
			eprintf (ofile, " IN FILE %s\n", ifn);
		if (fseek (ifile, sh->_s.s_relptr, 0) != 0)
			error ("cannot seek to relocation entries in section %s", secname);
		j = 0;
		while (j < (int) sh->_s.s_nreloc) {
			if (freads ((char *)&re, RELSZ, 1, ifile) != 1)
				error ("cannot read relocation entry %d in section %s", j, secname);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "r_vaddr = 0x%08lX  ",
				       re.r_vaddr);
			if (!verbose)
				eprintf (ofile, "r_symndx = %ld\n",
					       re.r_symndx);
			else {
				if (re.r_symndx < (long)sizeof (str_length) ||
				    re.r_symndx > str_length)
					error ("invalid string table offset for relocation entry %d in section %s", j, secname);
				eprintf (ofile, "r_symndx = %s\n",
					       &str_tab[re.r_symndx - sizeof (str_length)]);
			}
			j++;
		}
	}
}

static void
dump_le (XCNHDR *sh, int sn)
{
	char	*secname;
	LINENO	le;		/* Line number entry structure */
	int	j;

	if (sh->_s.s_lnnoptr && sh->_s.s_nlnno) {
		secname = get_secname (sh);
		eprintf (ofile, "\nLINE NUMBER ENTRIES FOR SECTION %s (%d)", secname, sn);
		if (test)
			eprintf (ofile, "\n");
		else
			eprintf (ofile, " IN FILE %s\n", ifn);
		if (fseek (ifile, sh->_s.s_lnnoptr, 0) != 0)
			error ("cannot seek to line number entries in section %s", secname);
		j = 0;
		while (j < (int) sh->_s.s_nlnno) {
			if (freads ((char *)&le, LINESZ, 1, ifile) != 1)
				error ("cannot read line number entry %d in section %s", j, secname);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "l_lnno = % 4lu  ", le.l_lnno);
			if (!le.l_lnno)
				eprintf (ofile, "l_symndx = %ld\n",
					       le.l_addr.l_symndx);
			else if (!verbose ||
				 CORE_ADDR_MAP (le.l_addr.l_paddr) < 0 ||
				 CORE_ADDR_MAP (le.l_addr.l_paddr) >= memstrlen)
				eprintf (ofile, "l_paddr  = 0x%08lX 0x%08lX\n",
					       CORE_ADDR_MAP (le.l_addr.l_paddr),
					       CORE_ADDR_ADDR (le.l_addr.l_paddr));
			else
				eprintf (ofile, "l_paddr  = %s0x%08lX\n",
					       memstr[CORE_ADDR_MAP (le.l_addr.l_paddr)],
					       CORE_ADDR_ADDR (le.l_addr.l_paddr));
			j++;
		}
	}
}

static void 
dump_symbols (void)
{
	SYMENT	se;
	AUXENT	ae;
	int	i, j;

	if (!symptr || !num_symbols)	/* no symbols */
		return;

	eprintf (ofile, "\nSYMBOL TABLE");
	if (test)
		eprintf (ofile, "\n");
	else
		eprintf (ofile, " FOR FILE %s\n", ifn);
	if (fseek (ifile, symptr, 0) != 0)
		error ("cannot seek to symbol table");
	i = 0;
	while (i < num_symbols) {
#if defined (MPW)
		SpinCursor (4);
#endif
		if (freads ((char *)&se, sizeof (SYMENT), 1, ifile) != 1)
			error ("cannot read symbol table entry %d", i);
		dump_se (&se, i);
		i++;
		for (j = 0; j < se.n_numaux; j++) {
			if (freads ((char *)&ae, sizeof (AUXENT), 1, ifile) != 1)
				error ("cannot read auxiliary entry %d for symbol entry %d", j, i);
			dump_ae (&se, &ae, i, j);
			i++;
		}
	}
}

static void
dump_se (SYMENT *se, int sc)
{
	char	*name, *scode, *sclass;
	unsigned long dt;

	if (se->n_zeroes) {
#if !defined (BIG_ENDIAN)
		swapw (se->n_name, sizeof (long), 2);
#endif
		name = se->n_name;
	} else {
		if (se->n_offset < (long)sizeof (str_length) ||
		    se->n_offset > str_length)
			error ("invalid string table offset for symbol table entry %d name", sc);
		name = &str_tab[se->n_offset - sizeof (str_length)];
	}
	eprintf (ofile, "%-6d  ", sc);
	eprintf (ofile, "n_name = %-16s  ", name);

	switch (se->n_sclass) {
	case C_EFCN:
		sclass = "C_EFCN";
		break;
	case C_NULL:
		sclass = "C_NULL";
		break;
	case C_AUTO:
		sclass = "C_AUTO";
		break;
	case C_EXT:
		sclass = "C_EXT";
		break;
	case C_STAT:
		sclass = "C_STAT";
		break;
	case C_REG:
		sclass = "C_REG";
		break;
	case C_EXTDEF:
		sclass = "C_EXTDEF";
		break;
	case C_LABEL:
		sclass = "C_LABEL";
		break;
	case C_ULABEL:
		sclass = "C_REG";
		break;
	case C_MOS:
		sclass = "C_MOS";
		break;
	case C_ARG:
		sclass = "C_ARG";
		break;
	case C_STRTAG:
		sclass = "C_STRTAG";
		break;
	case C_MOU:
		sclass = "C_MOU";
		break;
	case C_UNTAG:
		sclass = "C_UNTAG";
		break;
	case C_TPDEF:
		sclass = "C_TPDEF";
		break;
	case C_USTATIC:
		sclass = "C_USTATIC";
		break;
	case C_ENTAG:
		sclass = "C_ENTAG";
		break;
	case C_MOE:
		sclass = "C_MOE";
		break;
	case C_REGPARM:
		sclass = "C_REGPARM";
		break;
	case C_FIELD:
		sclass = "C_FIELD";
		break;
	    case C_MEMREG:
		sclass = "C_MEMREG";
		break;
	case C_OPTIMIZED:
		sclass = "C_OPTIMIZED";
		break;
	case C_BLOCK:
		sclass = "C_BLOCK";
		break;
	case C_FCN:
		sclass = "C_FCN";
		break;
	case C_EOS:
		sclass = "C_EOS";
		break;
	case C_FILE:
		sclass = "C_FILE";
		break;
	case C_LINE:
		sclass = "C_LINE";
		break;
	case C_ALIAS:
		sclass = "C_ALIAS";
		break;
	case C_HIDDEN:
		sclass = "C_HIDDEN";
		break;
	case C_SECT:
		sclass = "C_SECT";
		break;
	case C_SDI:
		sclass = "C_SDI";
		break;
	case A_FILE:
		sclass = "A_FILE";
		break;
	case A_SECT:
		sclass = "A_SECT";
		break;
	case A_BLOCK:
		sclass = "A_BLOCK";
		break;
	case A_MACRO:
		sclass = "A_MACRO";
		break;
	case A_GLOBAL:
		sclass = "A_GLOBAL";
		break;
	case A_XDEF:
		sclass = "A_XDEF";
		break;
	case A_XREF:
		sclass = "A_XREF";
		break;
	case A_SLOCAL:
		sclass = "A_SLOCAL";
		break;
	case A_ULOCAL:
		sclass = "A_ULOCAL";
		break;
	case A_MLOCAL:
		sclass = "A_MLOCAL";
		break;
	default:
		sclass = "<unknown>";
		break;
	}
	if (!verbose)
		eprintf (ofile, "n_value = 0x%08lX 0x%08lX\n",
			 CORE_ADDR_MAP (se->n_value),
			 CORE_ADDR_ADDR (se->n_value));
	else {
		if (BTYPE(se->n_type) == T_FLOAT) {
			union dbl d;
			d.l.hval = CORE_ADDR_MAP (se->n_value);
			d.l.lval = CORE_ADDR_ADDR (se->n_value);
			eprintf (ofile, "n_value = %-.6E\n", d.dval);
		} else if (BTYPE(se->n_type) == T_LONG)
			eprintf (ofile, "n_value = 0x%08lX 0x%08lX\n",
				 CORE_ADDR_MAP (se->n_value),
				 CORE_ADDR_ADDR (se->n_value));
		else if (IS_MAP (se->n_value) &&
			 CORE_ADDR_MAP (se->n_value) != memory_map_none &&
			 CORE_ADDR_MAP (se->n_value) >= 0 &&
			 CORE_ADDR_MAP (se->n_value) < memstrlen)
			eprintf (ofile, "n_value = %s0x%08lX\n",
				 memstr[CORE_ADDR_MAP (se->n_value)],
				 CORE_ADDR_ADDR (se->n_value));
		else
			eprintf (ofile, "n_value = 0x%08lX\n",
				 CORE_ADDR_ADDR (se->n_value));
	}
	eprintf (ofile, "%s", indent);
	eprintf (ofile, "%s", indent);
	switch (se->n_scnum) {
	case N_DEBUG:
		scode = "N_DEBUG";
		break;
	case N_ABS:
		scode = "N_ABS";
		break;
	case N_UNDEF:
		scode = "N_UNDEF";
		break;
	default:
		scode = "";
		break;
	}
	if (verbose && *scode)
		eprintf (ofile, "n_scnum = %s  ", scode);
	else
		eprintf (ofile, "n_scnum = % 5d  ", se->n_scnum);

	if (!verbose ||
	    ((se->n_sclass == C_FILE || se->n_sclass == A_FILE) &&
	     (BTYPE_INDEX (se->n_type) < 0 || ((int)BTYPE_INDEX (se->n_type) > ftypstrlen))) ||
	    (!(se->n_sclass == C_FILE || se->n_sclass == A_FILE) &&
	     (BTYPE_INDEX (se->n_type) < 0 || ((int)BTYPE_INDEX (se->n_type) > typstrlen))))
		eprintf (ofile, "n_type = 0%lo (0x%lX)  ",
			       se->n_type, se->n_type);
	else {
		eprintf (ofile, "n_type = %s",
			 se->n_sclass == C_FILE || se->n_sclass == A_FILE ?
			 ftypstr[BTYPE_INDEX(se->n_type)] :
			 typstr[BTYPE_INDEX(se->n_type)]);
		for (dt = se->n_type & ~N_BTMASK; dt; dt = DECREF (dt)) {
			if (ISFCN (dt))
				eprintf (ofile, ",DT_FCN");
			else if (ISPTR (dt))
				eprintf (ofile, ",DT_PTR");
			else if (ISARY (dt)) {
				eprintf (ofile, ",DT_ARY");
				isarray = YES;	/* set global array flag */
			}
		}
		eprintf (ofile, "  ");
	}

	if (!verbose)
		eprintf (ofile, "n_sclass = %ld  ", se->n_sclass);
	else
		eprintf (ofile, "n_sclass = %s  ", sclass);

	eprintf (ofile, "n_numaux = %ld\n", se->n_numaux);
}

static void
dump_ae (SYMENT *se, AUXENT *ae, int sc, int ac)
{
	int	i, j;
	char	*p = (char *)ae;
	char	*fname;

	eprintf (ofile, "%-6d  ", sc);
	if (!verbose) {
#if !defined (BIG_ENDIAN)
		swapw (p, 1, AUXESZ);
#endif
		for (i = 0; i < AUXESZ; i++) {
			eprintf (ofile, "%1X", (*p >> 4) & 0xf);
			eprintf (ofile, "%1X ", *p & 0xf);
			p++;
		}
		eprintf (ofile, "\n");
	} else {
		if (se->n_sclass == C_FILE || se->n_sclass == A_FILE) {
			if (!ae->x_file.x_foff) {
#if !defined (BIG_ENDIAN)
				swapw (ae->x_file.x_fname, 1, FILNMLEN);
#endif
				fname = ae->x_file.x_fname;
			} else {
				if (ae->x_file.x_foff < (long)sizeof (str_length) ||
				    (long) ae->x_file.x_foff > str_length)
					error ("invalid string table offset for file name");
				fname = &str_tab[ae->x_file.x_foff - sizeof (str_length)];
			}
			if (test)
				norm_fname (fname);
			eprintf (ofile, "fname = %s  ", fname);
			if (ae->x_file.x_ftype)
				eprintf (ofile, "x_ftype = 0x%04lX  ",
					       ae->x_file.x_ftype);
		} else if ((se->n_sclass == C_STAT &&	/* section */
		     BTYPE (se->n_type) == T_NULL)) {
			dump_sec (ae, ac);
		} else if (ISTAG (se->n_sclass)) {	/* tag */
			eprintf (ofile, "size = %4lu  ",
				       ae->x_sym.x_misc.x_lnsz.x_size);
			eprintf (ofile, "endndx = %6ld  ",
				       ae->x_sym.x_fcnary.x_fcn.x_endndx);
		} else if (se->n_sclass == C_EOS) {	/* end of structure */
			eprintf (ofile, "tagndx = %6ld  ",
				       ae->x_sym.x_tagndx);
			eprintf (ofile, "size = %4d  ",
				       ae->x_sym.x_misc.x_lnsz.x_size);
		} else if (se->n_sclass == C_FCN ||
			   se->n_sclass == C_BLOCK) {	/* function/block */
			eprintf (ofile, "lnno = %4ld  ",
				       ae->x_sym.x_misc.x_lnsz.x_lnno);
			eprintf (ofile, "endndx = %6ld  ",
				       ae->x_sym.x_fcnary.x_fcn.x_endndx);
			if (se->n_sclass == C_FCN &&
			    ae->x_sym.x_fcnary.x_fcn.x_type)
				eprintf (ofile, "x_type = 0x%04lX  ",
					       ae->x_sym.x_fcnary.x_fcn.x_type);
		} else if (se->n_sclass == A_SECT) {	/* assembly language section */
			eprintf (ofile, "tagndx = %6ld  ",
				       ae->x_sym.x_tagndx);
			eprintf (ofile, "lnno = %4ld  ",
				       ae->x_sym.x_misc.x_lnsz.x_lnno);
			eprintf (ofile, "endndx = %6ld  ",
				       ae->x_sym.x_fcnary.x_fcn.x_endndx);
		} else if (se->n_sclass == A_MACRO) {	/* assembly language macro */
			eprintf (ofile, "lnno = %4ld  ",
				       ae->x_sym.x_misc.x_lnsz.x_lnno);
			eprintf (ofile, "endndx = %6ld  ",
				       ae->x_sym.x_fcnary.x_fcn.x_endndx);
		} else if (se->n_sclass == C_FIELD) {	/* bit field */
			eprintf (ofile, "size = %4lu  ",
				       ae->x_sym.x_misc.x_lnsz.x_size);
		} else if (ISFCN (se->n_type)) {	/* function */
			eprintf (ofile, "tagndx = %6ld  ",
				       ae->x_sym.x_tagndx);
			eprintf (ofile, "fsize = %6ld  ",
				       ae->x_sym.x_misc.x_fsize);
			eprintf (ofile, "lnnoptr = %6ld  ",
				       ae->x_sym.x_fcnary.x_fcn.x_lnnoptr);
			eprintf (ofile, "endndx = %6ld  ",
				       ae->x_sym.x_fcnary.x_fcn.x_endndx);
		} else if (isarray) {	/* array */
			isarray = NO;	/* reset flag */
			eprintf (ofile, "tagndx = %6ld  ",
				       ae->x_sym.x_tagndx);
			eprintf (ofile, "lnno = %4ld  ",
				       ae->x_sym.x_misc.x_lnsz.x_lnno);
			eprintf (ofile, "size = %4lu  ",
				       ae->x_sym.x_misc.x_lnsz.x_size);
			for (j = 0; j < 4; j++) {
				if (!ae->x_sym.x_fcnary.x_ary.x_dimen[j])
					break;	/* no more dimensions */
				eprintf (ofile, "dimen[%d] = %4lu  ",
					       j, ae->x_sym.x_fcnary.x_ary.x_dimen[j]);
			}
		} else if (se->n_type == T_STRUCT ||	/* structure */
			   se->n_type == T_UNION  ||	/* union */
			   se->n_type == T_ENUM) {	/* enumeration */
			eprintf (ofile, "tagndx = %6ld  ",
				       ae->x_sym.x_tagndx);
			eprintf (ofile, "size = %4d",
				       ae->x_sym.x_misc.x_lnsz.x_size);
		}
		eprintf (ofile, "\n");
	}
}

static void 
norm_fname (char *fn)
{
	char *p, *q;
	int indir = NO;

	switch (*fn) {	/* check initial character */
	case '[':		/* VMS directory */
		indir = YES;
		/* fall through */
	case ':':		/* Mac relative path */
		for (p = fn, q = fn + 1; *q; p++, q++)
			*p = *q;	/* shift name over */
		*p = EOS;
		/* fall through */
	default:
		break;
	}
	for (p = fn; *p; p++)
		if (*p == '\\' ||	/* PC */
		    *p == ':'  ||	/* Mac */
		    (*p == '.' && indir))	/* VMS */
			*p = '/';	/* normalize to Unix */
		else if (*p == ']') {
			indir = NO;
			*p = '/';
		}
}


static void
dump_sec (AUXENT *ae, int ac)
{
	AUXLNK	*al;
	AUXLNK1	*al1;
	AUXLNK2	*al2;
	static long flags;

	switch (ac) {

	case 0:		/* first auxiliary entry */

		eprintf (ofile, "scnlen = %6ld  ", ae->x_scn.x_scnlen);
		eprintf (ofile, "nreloc = %4lu  ", ae->x_scn.x_nreloc);
		eprintf (ofile, "nlinno = %4lu  ", ae->x_scn.x_nlinno);
		break;

	case 1:		/* second auxiliary entry */

		if (!post4_1) {	/* old object file format */
			al = (AUXLNK *)ae;
			eprintf (ofile, "secno = %4ld  ", al->aux.secno);
			eprintf (ofile, "rsecno = %4ld  ", al->aux.rsecno);
			eprintf (ofile, "mem = 0x%04lX  ", al->aux.mem);
			eprintf (ofile, "flags = 0x%08lX  ", al->aux.flags);
			if (al->aux.flags & (BUF | OVL)) {
				eprintf (ofile, "\n");
				eprintf (ofile, "%s", indent);
				eprintf (ofile, "%s", indent);
				if (al->aux.flags & BUF) {
					eprintf (ofile, "bufcnt = %4ld  ", al->aux.bufovl.buf.bufcnt);
					eprintf (ofile, "buftyp = 0x%04lX  ", al->aux.bufovl.buf.buftyp);
					eprintf (ofile, "buflim = %6ld  ", al->aux.bufovl.buf.buflim);
				} else if (al->aux.flags & OVL) {
					eprintf (ofile, "ovlcnt = %4ld  ", al->aux.bufovl.ovl.ovlcnt);
					eprintf (ofile, "ovlmem = 0x%04lX  ", al->aux.bufovl.ovl.ovlmem);
					eprintf (ofile, "ovlstr = %6ld  ", al->aux.bufovl.ovl.ovlstr);
				}
			}
		} else {	/* not older object file format */
			al1 = (AUXLNK1 *)ae;
			/* save flags for subsequent call */
			flags = al1->aux.flags;
			eprintf (ofile, "secno = %4ld  ", al1->aux.secno);
			eprintf (ofile, "rsecno = %4ld  ", al1->aux.rsecno);
			eprintf (ofile, "flags = 0x%08lX  ", al1->aux.flags);
			eprintf (ofile, "\n");
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "mspace = %4ld  ",
				 al1->aux.mem.mspace);
			eprintf (ofile, "mmap = %4ld  ", al1->aux.mem.mmap);
			eprintf (ofile, "mcntr = %4ld  ", al1->aux.mem.mcntr);
			eprintf (ofile, "mclass = %4ld  ",
				 al1->aux.mem.mclass);
		}
		break;

	case 2:		/* third auxiliary entry */

		al2 = (AUXLNK2 *)ae;
		if (flags & BUF) {
			eprintf (ofile, "bufcnt = %4ld  ",
				 al2->bufovl.buf.bufcnt);
			eprintf (ofile, "buftyp = 0x%04lX  ",
				 al2->bufovl.buf.buftyp);
			eprintf (ofile, "buflim = %6ld  ",
				 al2->bufovl.buf.buflim);
		} else if (flags & OVL) {
			eprintf (ofile, "ovlcnt = %4ld  ",
				 al2->bufovl.ovl.ovlcnt);
			eprintf (ofile, "ovlstr = %6ld  ",
				 al2->bufovl.ovl.ovlstr);
			eprintf (ofile, "ovloff = 0x%08lX  ",
				 al2->bufovl.ovl.ovloff);
			eprintf (ofile, "\n");
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "%s", indent);
			eprintf (ofile, "ovlmem.mspace = %4ld  ",
				 al2->bufovl.ovl.ovlmem.mspace);
			eprintf (ofile, "ovlmem.mmap = %4ld  ",
				 al2->bufovl.ovl.ovlmem.mmap);
			eprintf (ofile, "ovlmem.mcntr = %4ld  ",
				 al2->bufovl.ovl.ovlmem.mcntr);
			eprintf (ofile, "ovlmem.mclass = %4ld  ",
				 al2->bufovl.ovl.ovlmem.mclass);
		}
		break;

	case 3:		/* fourth auxiliary entry */

		al2 = (AUXLNK2 *)ae;
		eprintf (ofile, "ovlcnt = %4ld  ", al2->bufovl.ovl.ovlcnt);
		eprintf (ofile, "ovlstr = %6ld  ", al2->bufovl.ovl.ovlstr);
		eprintf (ofile, "ovloff = 0x%08lX  ", al2->bufovl.ovl.ovloff);
		eprintf (ofile, "\n");
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "%s", indent);
		eprintf (ofile, "ovlmem.mspace = %4ld  ",
			 al2->bufovl.ovl.ovlmem.mspace);
		eprintf (ofile, "ovlmem.mmap = %4ld  ",
			 al2->bufovl.ovl.ovlmem.mmap);
		eprintf (ofile, "ovlmem.mcntr = %4ld  ",
			 al2->bufovl.ovl.ovlmem.mcntr);
		eprintf (ofile, "ovlmem.mclass = %4ld  ",
			 al2->bufovl.ovl.ovlmem.mclass);
		break;
	}
}

static void 
dump_strings (void)
{
	char	*str_ptr;
	long	offset = 0L;
	int	len;

	if (str_length) {
		eprintf (ofile, "\nSTRING TABLE");
		if (test)
			eprintf (ofile, "\n");
		else
			eprintf (ofile, " FOR FILE %s\n", ifn);
		eprintf (ofile, "%-8ld%ld  (string table length)\n",
			       offset, str_length);
		offset += sizeof (str_length);
		str_ptr = str_tab;
		do {
			eprintf (ofile, "%-8ld%s\n", offset, str_ptr);
			len = strlen (str_ptr);
			offset += len + 1;
			str_ptr += len + 1;
		} while (str_ptr < str_tab + str_length);
	}
}


/**
*
* name		cmdarg --- scan command line for argument
*
* synopsis	argp = cmdarg (arg, argc, argv)
*		char *argp;	pointer to argument
*		char arg;	argument to find
*		int argc;	count of command line arguments
*		char **argv;	pointer to command line arguments
*
* description	Takes a pointer to and count of the command line arguments.
*		Scans command line looking for argument character.  Returns
*		pointer to argument if found, NULL otherwise.
*
**/
static char *
cmdarg (char arg, int argc, char **argv)
{
	--argc;	/* skip over program name */
	++argv;
	while (argc > 0) {	/* scan args */
		if (**argv == '-') {
			char *p;
			for (p = *argv + 1; *p; p++)
				if ((isupper (*p) ? tolower (*p) : *p) == arg)
					return (*argv);
		}
		--argc;
		++argv;
	}
	return (NULL);
}


static int 
getopts (int argc, char *argv[], char *optstring)
/* get option letter from argv */
{
	register char c;
	register char *place;
	static char *scan = NULL;

	optarg = NULL;

	if (scan == NULL || *scan == '\0') {
		if (optind == 0)
			optind++;
	
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		if (strcmp(argv[optind], "--")==0) {
			optind++;
			return(EOF);
		}
	
		scan = argv[optind]+1;
		optind++;
	}

	c = *scan++;
	place = strchr(optstring, c);

	if (place == NULL || c == ':') {
		(void)fprintf(stderr, "%s: unknown option -%c\n", Progname, c);
		return('?');
	}

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			optarg = scan;
			scan = NULL;
		} else if (optind < argc) {
			optarg = argv[optind];
			optind++;
		} else {
			(void)fprintf( stderr, "%s: -%c argument missing\n", 
					Progname, c);
			return('?');
		}
	}

	return(c);
}


/**
* name		setfile --- set the file type and creator if necessary
*
* synopsis	yn = setfile (fn, type, creator)
*		int yn;		YES on success, NO on failure
*		char *fn;	pointer to file name
*		char *type;	pointer to file type
*		char *creator;	pointer to file creator
*
* description	Sets the file type and creator for newly-created Macintosh
*		output files.  Simply returns YES on other hosts.
*
**/
static int 
setfile (char *fn, char *type, char *creator)
{
#if defined (MPW)
	int i;
	short status;
	struct FInfo finfo;
	OSType val;
	static char buf[256];


	(void) strcpy (buf, fn);
	c2pstr(buf);
	if ((status = GetFInfo ((unsigned char *) &buf[0], (short)0, &finfo)) != 0)
		return (NO);
	i = 0;
	val = (OSType) 0;
	while (i < sizeof (OSType)) {
		val <<= (OSType) 8;
		val += (OSType) type[i++];
	}
	finfo.fdType = val;
	i = 0;
	val = (OSType) 0;
	while (i < sizeof (OSType)) {
		val <<= (OSType) 8;
		val += (OSType) creator[i++];
	}
	finfo.fdCreator = val;
	if ((status = SetFInfo ((unsigned char *) &buf[0], (short)0, &finfo)) != 0)
		return (NO);
#endif
#if defined (LINT)
	fn = fn;
	type = type;
	creator = creator;
#endif
	return (YES);
}

/**
*
* name		freads - swap bytes and read
*
* synopsis	freads (ptr, size, nitems, stream)
*		char *ptr;		pointer to buffer
*		int size;		size of buffer
*		int nitems;		number of items to read
*		FILE *stream;		file pointer
*
* description	Treats ptr as reference to union array; if necessary,
*		swaps bytes to maintain base format byte ordering
*		(big endian).  Calls fread to do I/O.
*
**/
static int 
freads (char *ptr, int size, int nitems, FILE *stream)
{
	int rc;

	rc = fread (ptr, size, nitems, stream);
#if !defined (BIG_ENDIAN)
	swapw (ptr, size, nitems);
#endif
	return (rc);
}

#if !defined (BIG_ENDIAN)
/**
*
* name		swapw - swap bytes in words
*
* synopsis	swapw (ptr, size, nitems)
*		char *ptr;		pointer to buffer
*		int size;		size of buffer
*		int nitems;		number of items to write
*
* description	Treats ptr as reference to union array; if necessary,
*		swaps bytes to maintain base format byte ordering
*		(big endian).
*
**/
static void 
swapw (char *ptr, int size, int nitems)
{
	union wrd *w;
	union wrd *end = (union wrd *)ptr +
		((size * nitems) / sizeof (union wrd));
	unsigned i;

	for (w = (union wrd *)ptr; w < end; w++) {
		i = w->b[0];
		w->b[0] = w->b[3];
		w->b[3] = i;
		i = w->b[1];
		w->b[1] = w->b[2];
		w->b[2] = i;
	}
}
#endif


/**
*
* name		dectime - decode broken down time from time_t value
*
* synopsis	timeptr = dectime (clock);
*		struct tm *timeptr;	pointer to tm structure
*		time_t clock;		time value to decode
*
* description	Converts the time_t value as returned by enctime()
*		into a tm structure representing the encoded
*		time.  Returns a pointer to a static structure.
*		No allowance is made for timezone or DST checking.
*
**/
static struct tm *
dectime (time_t *clock)
{
	register struct tm *	tmp;
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register int *		ip;
	static struct tm	tm;

	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = ISLEAP(y);
			if (days < (long) Year_lengths[yleap])
				break;
			++y;
			days = days - (long) Year_lengths[yleap];
		}
	else do {
		--y;
		yleap = ISLEAP(y);
		days = days + (long) Year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = Mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	return tmp;
}


/**
*
* name		onsig - handle signals
*
* description	Displays a message on the standard error output and
*		exits with the error count.
*
**/
static void
onsig (int sig)
{
	switch (sig) {
	case SIGINT:
#if defined (WATCOM)	/* Watcom doesn't like variadic calls in signal handlers */
		exit (1);
#else
		(void) fprintf (stderr, "\n%s: Interrupted\n", Progname);
#if defined (SIOW)
		longjmp (Env, 1);
#else
		exit (1);
#endif
#endif
		break;
	case SIGSEGV:
		(void) fprintf (stderr, "%s: Fatal segmentation or protection fault; contact Motorola\n", Progname);
		exit (1);
		break;
	}
}


static void 
usage (void)			/* display usage on stderr, exit nonzero */
{
#if defined (MSDOS) || defined (TOS)
	if (quiet)
#endif
		(void)fprintf (stderr, "%s  %s\n%s\n",
			       Progtitle, Version, Copyright);
	(void)fprintf (stderr, "Usage:  %s [-cfhloqrstv] [-d <file>] files\n",
		       Progname);
	(void)fprintf (stderr, "        c - dump string table\n");
	(void)fprintf (stderr, "        d - dump to output file\n");
	(void)fprintf (stderr, "        f - dump file header\n");
	(void)fprintf (stderr, "        h - dump section headers\n");
	(void)fprintf (stderr, "        l - dump line number information\n");
	(void)fprintf (stderr, "        o - dump optional header\n");
	(void)fprintf (stderr, "        q - do not display signon banner\n");
	(void)fprintf (stderr, "        r - dump relocation information\n");
	(void)fprintf (stderr, "        s - dump section contents\n");
	(void)fprintf (stderr, "        t - dump symbol table\n");
	(void)fprintf (stderr, "        v - dump symbolically\n");
	exit (1);
}

#if defined (LINT)
#if defined (va_dcl)
#undef va_dcl
#define va_dcl char *va_alist;
#endif
#endif

/* VARARGS */
static void
#if !defined (VARARGS)
eprintf (FILE *fp, char *fmt, ...)	/* call fprintf, check for errors */
#else
eprintf (va_alist)			/* call fprintf, check for errors */
va_dcl
#endif
{
	va_list ap;
#if !defined (VARARGS)
	va_start (ap, fmt);
#else
	FILE *fp;
	char *fmt;
	va_start (ap);
	fp = va_arg (ap, FILE *);
	fmt = va_arg (ap, char *);
#endif
	if (vfprintf (fp, fmt, ap) < 0)
		error ("cannot write to output file");
	va_end (ap);
}

/* VARARGS */
static void
#if !defined (VARARGS)
error (char *fmt, ...)		/* display error on stderr, exit nonzero */
#else
error (va_alist)		/* display error on stderr, exit nonzero */
va_dcl
#endif
{
	va_list ap;
#if !defined (LINT)
	int err = errno;
#endif
#if !defined (VARARGS)
	va_start (ap, fmt);
#else
	char *fmt;
	va_start (ap);
	fmt = va_arg (ap, char *);
#endif
	(void)fprintf  (stderr, "%s: ", Progname);
	(void)vfprintf (stderr, fmt, ap);
	(void)fprintf  (stderr, "\n");
	va_end (ap);
#if !defined (LINT)
	if (err) {
		errno = err;
		perror (Progname);
	}
#endif
	exit (1);
}

#ifdef	MSIL_EXTENSIONS
/*
 * Disassembly and symbolic information.
 *
 * The handling of symbols is incomplete. I just used some
 * code from the simulator. Disassembly could be done without
 * any symbols involved - it would be a lot less work and still
 * be useful.
 */
int
compare_sym_idx(idx1, idx2)
    struct sym_idx *idx1, *idx2;
{
    int i;

    if (i = (idx1->section - idx2->section))
	return i;
    return idx1->offset - idx2->offset;
}

find_closest_symbol(section, pc_val)	/* Returns index into symtab */
    unsigned long pc_val;
    int	section;
{
    int lo, hi, mid;
    int cmpval;
    struct sym_idx given_pc;

    given_pc.offset = pc_val;
    given_pc.section = section;

    cmpval = 1;
    lo = 0;
    hi = n_sorted_symbols - 1;

    while (lo <= hi) {
	mid = (lo + hi + 1)/2;
	cmpval = compare_sym_idx(&sorted_symbol_index[mid], &given_pc);
	if (cmpval == 0)
	    break;
	if (cmpval > 0)
	    hi = mid - 1;
	else
	    lo = mid + 1;
    }
    if (cmpval)
	mid = lo;

    if (sorted_symbol_index[mid].section == section)
	return mid;
    else
	return -1;	/* No previous symbol found for this section */
}

char *
find_name_of_symbol(symidx)
    int symidx;
{
    SYMENT *se = &symtab[symidx];
    char *name;

    if (symidx == -1)
	return "";

    if (se->n_zeroes) {
#if I8086 || VAX || D3100
	    swapw (se->n_name, sizeof (long), 2);
#endif
	    name = se->n_name;
    } else {
	    if (se->n_offset < sizeof (str_length) ||
		se->n_offset > str_length)
		    error ("invalid string table offset for symbol table entry %d name", se - symptr);
	    name = &str_tab[se->n_offset - sizeof (str_length)];
    }
    return name;
}

read_symbols ()
{
    int i,j;

    if (!symptr || !num_symbols)	/* no symbols */
	return(0);
    
    if (fseek (ifile, symptr, 0) != 0){
	error ("cannot seek to symbol table");
	return(-1);
	}
    
    symtab = (SYMENT *)malloc((unsigned)(num_symbols * sizeof(SYMENT)));
    sorted_symbol_index = (struct sym_idx *)
		malloc((unsigned)(num_symbols * sizeof(struct sym_idx)));
    if (!symtab || !sorted_symbol_index){
	error ("cannot allocate space for symbol table");
	return(-1);
    }
    
    if (freads ((char *)symtab, sizeof(SYMENT), (int)num_symbols, ifile) != 
	    num_symbols){
	error ("cannot read symbol table entries");
	return(-1);
    }

    for (j = 0, i = 0; i < num_symbols;) {
	char *ptr;
	
	if ((symtab[i].n_sclass == C_EXT || symtab[i].n_sclass == C_STAT ||
	    symtab[i].n_sclass == C_LABEL) &&
		((symtab[i].n_type == T_INT &&
		    (find_name_of_symbol(i)[0] == 'F'))
		    || ISFCN(symtab[i].n_type))) {
	    sorted_symbol_index[j].index = i;
	    sorted_symbol_index[j].section = symtab[i].n_scnum;
	    sorted_symbol_index[j].offset = CORE_ADDR_ADDR(symtab[i].n_value);
	    j++;
	}
	i += symtab[i].n_numaux + 1;
    }
    n_sorted_symbols = j;
    
    qsort( (char *) sorted_symbol_index,
		n_sorted_symbols, sizeof(struct sym_idx), compare_sym_idx);

    /* some fields need to be unbyteswapped on some machines */
    
#if !BIG_ENDIAN
    {
	int	i;
	for (i=0;i < sv_var->db.num_symbols;) {
	    SYMENT	*se;
	    AUXENT	*ae;
	    se= symtab[i];
	    
	    if (se->n_zeroes) {
		swapw (se->n_name, sizeof (long), 2);
	    }
	    
	    if (se->n_sclass == A_FILE||se->n_sclass == C_FILE) {
		ae= symtab[i+1]; 	/* points to first auxiliary entry */
		if (!ae->x_file.x_foff) {
		    swapw (ae->x_file.x_fname, 1, AUXESZ);
		}
	    }
	    i+= se->n_numaux+1;
	}
    }
#endif
    return (0);
}

#define	CUR_PC	(CORE_ADDR_ADDR(sh->_s.s_paddr) + next_word)
#define FOLLOWING_SECTION(I,SN)	((I) != -1 && (I)+1 < n_sorted_symbols && \
			sorted_symbol_index[(I)+1].section == SN)

/*
 * The disassembly main loop.
 * Invokes dspt_unasm_xxx() from the simulator library.
 * Disassembly without the symbol information would be just
 * a simple loop.
 */
disassemble_text(sh, sn, raw_data)
    XCNHDR *sh;
    int sn;
    long    *raw_data;
{
    unsigned long srval,omrval;
    long next_word = 0;
    char strp[MAXSTR];
    char *cur_func_name;

    int cur_func_idx = -1;
    unsigned long func_end = 0;

    cur_func_idx = find_closest_symbol(sn, CUR_PC);
    cur_func_name =
	find_name_of_symbol(sorted_symbol_index[cur_func_idx].index);

    if (FOLLOWING_SECTION(cur_func_idx, sn)) {
	func_end = sorted_symbol_index[cur_func_idx+1].offset;
    }
    else
	func_end = ~0;

    eprintf (ofile,"\n");
    srval = omrval = 0L;

    while (next_word < sh->_s.s_size) {
	int cur_insn_len;

	if (CUR_PC >= func_end) {

	    if (FOLLOWING_SECTION(cur_func_idx, sn)) {
		cur_func_idx++;
		cur_func_name =
		    find_name_of_symbol(sorted_symbol_index[cur_func_idx].index);
		if (FOLLOWING_SECTION(cur_func_idx, sn)) {
		    func_end = sorted_symbol_index[cur_func_idx+1].offset;
		}
		else
		    func_end = ~0;
	    }
	    else
		cur_func_idx = -1;
	}
	switch (file_header.f_magic) {
	case M56KMAGIC:
		cur_insn_len = dspt_unasm_56k(&raw_data[next_word],
					      strp, srval, omrval, NULL);
		break;
	case M96KMAGIC:
		cur_insn_len = dspt_unasm_96k(raw_data[next_word],
					      raw_data[next_word+1],
					      strp, srval, omrval, NULL);
		break;
	case M16KMAGIC:
		cur_insn_len = dspt_unasm_56100(raw_data[next_word],
						raw_data[next_word+1],
						strp, srval, omrval, NULL);
		break;
	case M563MAGIC:
		cur_insn_len = dspt_unasm_563(&raw_data[next_word],
					      strp, srval, omrval, NULL);
		break;
	case M568MAGIC:
		cur_insn_len = dspt_unasm_56800(&raw_data[next_word],
						strp, srval, omrval, NULL);
		break;
	case M566MAGIC:
		cur_insn_len = dspt_unasm_56600(&raw_data[next_word],
					      strp, srval, omrval, NULL);
		break;                
	case M569MAGIC:
		cur_insn_len = dspt_unasm_56900(&raw_data[next_word],
						strp, srval, omrval, NULL);
		break;
	default:
		break;
	}
	if (cur_insn_len == 0)
	    cur_insn_len = 1;
#if 0	/* jettison this for the time being */
	if (cur_func_idx != -1)
	    eprintf(ofile, "%s+%-6dP:%08lX ", cur_func_name,
		    CUR_PC-sorted_symbol_index[cur_func_idx].offset, CUR_PC);
	else
	    eprintf(ofile, "%s%sP:%08lX ", indent, indent, CUR_PC);
#else
	eprintf(ofile, "%sP:%08lX ", indent, CUR_PC);
#endif
	if (cur_insn_len == 1)
	    eprintf(ofile, "%08lX % 8s",  raw_data[next_word], " ");
	else
	    eprintf(ofile, "%08lX %08lX", raw_data[next_word],
		    raw_data[next_word+1]);
	eprintf(ofile, "%s%s\n", indent, strp);
	next_word += cur_insn_len;
    }
}
#endif /* MSIL_EXTENSIONS */ 
