/* $Id: srec.h,v 1.16 1997/06/27 19:52:01 jay Exp $ */	/* RCS data */

/**
*	SREC Definitions
**/

#define TRUE		1
#define FALSE		0
#define YES		1
#define NO		0
#define EOS		'\0'		/* end-of-string constant */
#define MAXSTR		512		/* maximum string size + 1 */
#define MAXFLD		16		/* maximum field value size */
#define MAXEXTLEN	4		/* longest filename extension */

#if defined (VMS)				/* exit status values */
#define OK	0x18000001L
#define	ERR	0x1800FFFBL
#define CLI_ABSENT	CLI$_ABSENT
#else
#define OK	0
#define ERR	(-1)
#define CLI_ABSENT	0
#endif

#define DSP56000	1		/* DSP56000 flag constant */
#define DSP96000	2		/* DSP96000 flag constant */
#define DSP56100	3		/* DSP56100 flag constant */
#define DSP56300	4		/* DSP56300 flag constant */
#define DSP56800	5		/* DSP56800 flag constant */
#define DSP56600        6               /* DSP56600 flag constang */
#define DSP56900        7               /* DSP56900 flag constang */

#define ASIZE1		2		/* S1/S9 record address size */
#define ASIZE2		3		/* S2/S8 record address size */
#define ASIZE3		4		/* S3/S7 record address size */

#define WSIZE2		2		/* 2 byte DSP word size */
#define WSIZE3		3		/* 3 byte DSP word size */
#define WSIZE4		4		/* 4 byte DSP word size */

#define MASK2		0xFFFFL		/* 2 byte word mask */
#define MASK3		0xFFFFFFL	/* 3 byte word mask */
#define MASK4		0xFFFFFFFFL	/* 4 byte word mask */

#define FMT4		"%04lx"		/* 4-digit word/addr format string */
#define FMT6		"%06lx"		/* 6-digit word/addr format string */
#define FMT8		"%08lx"		/* 8-digit word/addr format string */

#define S1FMT		"S1%02lx%04lx%s%02x\n"	/* S1 format string */
#define S2FMT		"S2%02lx%06lx%s%02x\n"	/* S2 format string */
#define S3FMT		"S3%02lx%08lx%s%02x\n"	/* S3 format string */
#define S7FMT		"S7%02lx%s%02x\n"	/* S7 format string */
#define S8FMT		"S8%02lx%s%02x\n"	/* S8 format string */
#define S9FMT		"S9%02lx%s%02x\n"	/* S9 format string */

#define S0OVRHD		3		/* S0 record overhead */
#define MAXBYTE		30		/* max data bytes per S-record */
#define MAXOVRHD	8		/* maximum S-record overhead */
#define MAXBUF	(MAXBYTE + MAXOVRHD) * 2/* maximum byte buffer size */
#define CSMASK		0xFF		/* checksum mask */

#define MSPACES	6			/* number of memory spaces */
#define XMEM	0			/* memory space array offsets */
#define YMEM	1
#define LMEM	2
#define PMEM	3
#define EMEM	4
#define DMEM	5

#define NONE	0			/* OMF record codes */
#define START	1
#define END	2
#define DATA	3
#define BDATA	4
#define SYMBOL	5
#define COMMENT	6

#define RECORD	1			/* OMF field types */
#define HEXVAL	2

#define NEWREC	'_'			/* new record indicator */

/*	File type designations	*/
#define FT_UNKNOWN	(-1)
#define FT_NONE		0
#define FT_LOD		1
#define FT_CLD		2

struct srec {				/* S-record structure */
		FILE *fp;
		unsigned checksum;
		char *p;
		char buf[MAXBUF + 1];
		};

union wrd {	/* word union for byte swapping */
	unsigned long l;
	unsigned char b[4];
};

extern int main (int argc, char *argv[]);
static int open_ifile (char *fn);
static void do_srec (FILE *fp);
static void do_coff (FILE *fp);
static void read_headers (FILE *fp);
static void read_data (XCNHDR *sh, int sn,int spc,int mem);
static void read_bdata (XCNHDR *sh, int sn,int spc,int mem);
static void do_end (void);
static void do_lod (FILE *fp);
static int get_start (FILE *fp);
static int get_record (void);
static void get_data (int spc, int mem);
static void get_bdata (int spc, int mem);
static void get_end (void);
static int sum_addr (unsigned long addr);
static void get_bytes (int space, char *fbp);
static void flush_rec (int space, unsigned long addr, unsigned long count);
static void check_addr (unsigned long addr);
static void rev_bytes (char *buf,int mem);
static void open_ofiles (int space);
static int get_field (void);
static int get_comment (void);
static int get_line (void);
static char *scan_field (register char *bp);
static void bld_s0 (unsigned space);
static int setup_mach (char *buf);
static int get_mem (enum memory_map mem);
static int get_space (void);
static int get_memch (int spc);
static get_ftype (char *fname);
static char *fix_fname (char *fn, char *ext);
static char *strup (char *str);
static int fldhex (void);
static char *basename (char *str);
static int setfile (char *fn, char *type, char *creator);
static int freads (char *ptr, int size, int nitems, FILE *stream);
#if !defined (BIG_ENDIAN)
static void swapw (char *ptr, int size, int nitems);
#endif
static char *cmdarg (char arg, int argc, char **argv);
int getopts (int argc, char *argv[], char *optstring);
#if defined (VMS)
static int dcl_getval (struct dsc$descriptor_s *opt);
static int dcl_reset (void);
#else
static int dcl_getval (int *opt);
#endif
static void onsig (int sig);
static void usage (void);
static void error (char *str);
static void error2 (char *fmt, char *str);
