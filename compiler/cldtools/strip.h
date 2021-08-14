/* $Id: strip.h,v 1.7 1993/05/26 20:06:47 tomc Exp $ */	/* RCS data */

/*
	STRIP - DSP COFF Strip File Utility Definitions
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
#define MAXBUF	512			/* maximum buffer size */
#define STREQ(a,b)	(*(a) == *(b) && strcmp((a), (b)) == 0)
#define BUFLEN	(MAXBUF * sizeof (long))	/* length of data buffer */

#if defined (VMS)
#define remove	delete
#endif
#if defined (UNIX)
#define remove	unlink
#endif

#if defined (APOLLO)
#if !defined (L_tmpnam)
#define L_tmpnam	32
#endif
#endif

#if !defined (FILHZ)
#define FILHZ	sizeof(FILHDR)
#endif

union wrd {	/* word union for byte swapping */
	unsigned long l;
	unsigned char b[4];
};

extern int main (int argc, char *argv []);
static void strip_file (void);
static char *cmdarg (char arg, int argc, char **argv);
static int getopts (int argc, char *argv [], char *optstring);
#if defined (APOLLO)
char *tmpnam (char *template);
#endif
static char *basename (char *str);
static int setfile (char *fn, char *type, char *creator);
static int freads (char *ptr, int size, int nitems, FILE *stream);
static int fwrites (char *ptr, int size, int nitems, FILE *stream);
#if !defined (BIG_ENDIAN)
static void swapw (char *ptr, int size, int nitems);
#endif
static void onsig (int sig);
static void usage (void);
#if !defined (VARARGS)
static void error ( char *fmt, ... );
#else
static void error ( /* int va_alist */ );
#endif
