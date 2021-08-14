/* $Id: cofdmp.h,v 1.12 1997/04/11 22:11:41 jay Exp $ */	/* RCS data */

/*
	COFDMP - DSP COFF Dump File Utility Definitions
*/

/*
  General-purpose defines
*/

#define TRUE	1
#define FALSE	0
#define YES	TRUE
#define NO	FALSE
#define EOS	'\0'			/* end-of-string constant */
#define MAXSTR	512			/* maximum string size + 1 */
#define STREQ(a,b)	(*(a) == *(b) && strcmp((a), (b)) == 0)
#define RAW	4	/* width of raw data display */
#define F_CC	0x00010000L	/* object file produced by C compiler */
#define BUF	0x2000	/* buffer symbol/section */
#define OVL	0x4000	/* overlay symbol/section */

#if !defined (FILHZ)
#define FILHZ	sizeof(FILHDR)
#endif

#define SECS_PER_MIN	60
#define MINS_PER_HOUR	60
#define HOURS_PER_DAY	24
#define DAYS_PER_WEEK	7
#define DAYS_PER_NYEAR	365
#define DAYS_PER_LYEAR	366
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define MONS_PER_YEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11
#define TM_SUNDAY	0

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY
#define EPOCH_BASE	(EPOCH_YEAR - TM_YEAR_BASE)

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define ISLEAP(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#define IS_MAP(x)	(CORE_ADDR_MAP (x) >= 0 && \
			 CORE_ADDR_MAP (x) < memory_map_maxp1)

union wrd {	/* word union for byte swapping */
	unsigned long l;
	unsigned char b[4];
};

union dbl {     /* double-precision floating point/integer union */
	double dval;
	struct {
#if defined (MSDOS) || defined (R3000C)
		unsigned long lval, hval;
#else
		unsigned long hval, lval;
#endif
	} l;
#if defined (VAX)
	struct {
		unsigned short lmval, mlval, mhval, hmval;
	} s;
#endif
};

extern int main (int argc, char *argv []);
static void clear_opts (void);
static void dump_file (void);
static void read_headers (void);
static void read_strings (void);
static void dump_filhdr (void);
static void dump_opthdr (void);
static void read_sections (void);
static char *get_secname (XCNHDR *sh);
static void dump_sh (XCNHDR *sh, int sn);
static void dump_data (XCNHDR *sh, int sn);
static void dump_re (XCNHDR *sh, int sn);
static void dump_le (XCNHDR *sh, int sn);
static void dump_symbols (void);
static void dump_se (SYMENT *se, int sc);
static void dump_ae (SYMENT *se, AUXENT *ae, int sc, int ac);
static void norm_fname (char *fn);
static void dump_sec (AUXENT *ae, int ac);
static void dump_strings (void);
static char *cmdarg (char arg,  int argc,  char **argv);
static int getopts (int argc, char *argv [], char *optstring);
static int setfile (char *fn, char *type, char *creator);
static int freads (char *ptr, int size, int nitems, FILE *stream);
#if !defined (BIG_ENDIAN)
static void swapw (char *ptr, int size, int nitems);
#endif
static struct tm *dectime (time_t *clock);
static void onsig (int sig);
static void usage (void);
#if !defined (VARARGS)
static void error (char *fmt, ...);
static void eprintf (FILE *fp, char *fmt, ...);
#else
static void error ( /* int va_alist */ );
static void eprintf ( /* FILE *fp, int va_alist */ );
#endif

#if 0
#define MSIL_EXTENSIONS		/* turn on MSIL extensions */
#endif
