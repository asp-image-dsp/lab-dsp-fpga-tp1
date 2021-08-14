/* $Id: cldlod.c,v 1.42 1998/01/07 19:39:50 jay Exp $ */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#if defined ( __WATCOMC__ ) || defined( __DGUX__ ) || defined( MPW )
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#if !defined ( MPW )
#include <sys/types.h>
#endif

/* Headers for working with COFF files */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0
#endif
#include "coreaddr.h"
#include "maout.h"
#include "dspext.h"

union dbl {     /* double-precision floating point/integer union */
        double dval;
        struct {
#if !BIG_ENDIAN
                unsigned long lval, hval;
#else
                unsigned long hval, lval;
#endif
        } l;
};

struct comment {	/* comment structure */
	long c_scno;
	long c_text;
	struct comment *c_next;
};

/* function definitions */
static void cld_to_lod();
static void read_headers();
static void collect_comments();
static void read_strings();
static void start_record();
static void read_sections();
static void dump_comments();
static void dump_data();
static void dump_deb_symbols (void);
static int freads (char *ptr, int size, int nitems,FILE *stream);
static void dump_se_d (SYMENT *se);

#if !BIG_ENDIAN
static void swapw (char *ptr, int size, int nitems);
#endif

#if defined ( __WATCOMC__ ) || defined( __DGUX__ ) || defined( MPW )
static void eprintf( FILE*, char*, ... );
static void error( char*, ... );
#else
static void eprintf();
static void error();
#endif

/*  Global variables  */
FILHDR	file_header;	/* File header structure */
AOUTHDR	opt_header;	/* Optional header structure */
OPTHDR	link_header;	/* Linker header structure */
int	absolute;	/* Absolute file flag */

long num_sections;	/* Number of sections */
long section_seek;	/* Used to seek to first section */

long symptr;		/* File pointer to symbol table entries */
long num_symbols;	/* Number of symbols */

int  data_width;        /* width of data for printing */
int  addr_width;        /* width of address for printing */

char *str_tab;		/* Pointer to start of string char. array */
long str_length;	/* Length in bytes of string array */

FILE *ifile = NULL;	/* file pointer for input file */
char *ifn = NULL;	/* pointer to input file name */

/* init is to non valid memory space */
int  space = 777; /* 0=p, 1=x, 2=y, 3=l, 4=N */

struct comment *com_head = NULL;	/* head of comment chain */
struct comment *cur_cmt = NULL;		/* current comment in chain */

static void
onintr (int signum)			/* clean up from signal */
{
    void exit ();

    exit (1);
}

void
main (argc, argv)
int argc;
char *argv[];
{
    void exit ();

    /* set up for signals, save program name, check for command line options */
    signal (SIGINT, onintr);

    /* check for correct command-line */
    if ( argc != 2 )
    {
	fprintf( stderr, "Version 6.2 usage: cldlod cldfile > lodfile\n" );
	exit ( -1 );
    }


    if ( ( ifile = fopen( argv[1], "rb" ) ) == NULL )
	error( "cannot open input file %s", argv[1] );

    cld_to_lod();
    
    fclose( ifile );
    exit (0);
}


static void
cld_to_lod()
{
    read_headers ();

    /* blow out the _START record */
    if ( symptr != 0  && num_symbols != 0 )	/* no symbols */
    {
	read_strings ();
	start_record();
	collect_comments ();
    }

    read_sections ();

    if ( symptr != 0  && num_symbols != 0 )	/* no symbols */
	dump_deb_symbols ();

    /* blow out the _END record */
    eprintf( stdout, "\n_END %01.*X\n", addr_width,
	    CORE_ADDR_ADDR (opt_header.entry) );
}


/* blow out the first .cmt symbol with:
        n_sclass == C_NULL
	n_type   == T_NULL
*/
static void
start_record()
{
    SYMENT se;
    int i = 0, sym_id = -709;
    
    if ( fseek( ifile, symptr, 0 ) != 0 )
	error ("cannot seek to symbol table");

    while ( i < num_symbols )
    {
	if ( freads( (char *)&se, sizeof (SYMENT), 1, ifile ) != 1 )
	    error ("cannot read symbol table entry %d", i);
#if !BIG_ENDIAN
	if ( se.n_zeroes )
		swapw( se.n_name, sizeof (long), 2 );
#endif
	if ( strcmp( se.n_name, ".cmt" ) == 0 &&
	     se.n_scnum == N_ABS &&
	     se.n_sclass == C_NULL &&
	     se.n_type == T_NULL )
	{
	    sym_id = CORE_ADDR_ADDR(se.n_value);
	    break;
	}
	
	i++;
    }
    
    /* */
    eprintf( stdout, "_START " );

    if ( sym_id >= 0 && str_length != 0 )
    {
	char *str_ptr = str_tab;
	char *cmt_ptr;
	int  len;
	long offset = (long) sizeof( str_length );
	
	do
	{
	    if ( offset == sym_id )
		break;
	    else
	    {
		len = strlen( str_ptr );
		offset += len + 1;
		str_ptr += len + 1;
	    }
	} while ( str_ptr < ( str_tab + str_length ) );

	cmt_ptr = strchr (str_ptr, ';');
	if (!cmt_ptr)
		eprintf( stdout, "%s\n\n", str_ptr );
	else
	{
		*cmt_ptr = 0;
		eprintf( stdout, "%s\n", str_ptr );
		*cmt_ptr = ';';
		eprintf( stdout, "%s\n", cmt_ptr );
	}
    }
    else
	eprintf( stdout, "\n\n" );
}


static void
read_headers ()
{
    if ( freads( (char *)&file_header, sizeof (FILHDR), 1, ifile ) != 1 )
	error ("cannot read file header");

    /* Save the global values */
    num_sections = file_header.f_nscns;
    num_symbols = file_header.f_nsyms;
    symptr = file_header.f_symptr;
    absolute = !!(file_header.f_flags & F_RELFLG);

    /* check the MAGIC number */
    if ( file_header.f_magic == M56KMAGIC )
    {
	data_width = 6;
	addr_width = 4;
    }
    else if ( file_header.f_magic == M96KMAGIC )
    {
	data_width = addr_width = 8;
    }
    else if ( file_header.f_magic == M16KMAGIC )
    {
	data_width = addr_width = 4;
    }
    else if ( file_header.f_magic == M563MAGIC )
    {
	data_width = addr_width = 6;
    }
    else if ( file_header.f_magic == M566MAGIC )
    {
	data_width = 6;
        addr_width = 4;
    }
    else if ( file_header.f_magic == M568MAGIC )
    {
	data_width = addr_width = 4;
    }
    else if ( file_header.f_magic == M569MAGIC )
    {
	data_width = 4;
	addr_width = 8;
    }
    else
    {
	error( "Header has a bad magic number" );
    }
    

    /* optional header present */
    if ( file_header.f_opthdr )
    {
	if ( absolute )
	{
	    if ( freads( (char *)&opt_header,
			(int)file_header.f_opthdr, 1, ifile ) != 1 )
		error( "cannot read optional file header" );
	}
	else
	{
	    if ( freads( (char *)&link_header,
			(int)file_header.f_opthdr, 1, ifile ) != 1 )
		error( "cannot read linker file header" );
	}
    }

    /* File offset for first section headers */
    section_seek = sizeof(FILHDR) + file_header.f_opthdr;
}


static void
read_strings ()
{
    long strings;

    strings = symptr + (num_symbols * SYMESZ);
    if ( fseek( ifile, strings, 0 ) != 0 )
	error ( "cannot seek to string table length" );
    if ( freads( (char *)&str_length, 4, 1, ifile ) != 1 && !feof( ifile ))
	error ( "cannot read string table length" );
    if ( feof( ifile ))
	str_length = 0L;
    else if ( str_length )
    {
	str_length -= 4;
	str_tab = (char *)malloc ((size_t)str_length);
	if ( !str_tab )
	    error( "cannot allocate string table" );
	if ( fseek( ifile, strings + 4, 0 ) != 0 )
	    error( "cannot seek to string table" );
	if (fread (str_tab, (int)str_length, 1, ifile) != 1)
	    error( "cannot read string table" );
    }
}

static void
collect_comments ()
{
    SYMENT se;
    AUXENT ae;
    int    i, j, k;

    if (fseek (ifile, symptr, 0) != 0)
	error ("cannot seek to symbol table");

    i = 0;
    while (i < num_symbols)
    {
	struct comment *cmnt;

	if (freads ((char *)&se, sizeof (SYMENT), 1, ifile) != 1)
	    error ("cannot read symbol table entry %d", i);

#if !BIG_ENDIAN
	if ( se.n_zeroes )
		swapw( se.n_name, sizeof (long), 2 );
#endif
	if ( strcmp( se.n_name, ".cmt" ) == 0 &&
	     se.n_scnum != N_ABS &&
	     se.n_sclass == C_NULL &&
	     se.n_type == T_NULL )
	{
	    struct comment *cp =
		(struct comment *)malloc (sizeof (struct comment));
	    if ( !cp )
	        error ( "cannot allocate comment record" );
	    cp->c_scno = se.n_scnum;
	    cp->c_text = CORE_ADDR_ADDR(se.n_value);
	    cp->c_next = NULL;
	    if ( !com_head )
	        com_head = cmnt = cp;
	    else
	        cmnt = cmnt->c_next = cp;
	}

	k = i++;

	for (j = 0; j < se.n_numaux; j++)
	{
	    if (freads ((char *)&ae, sizeof (AUXENT), 1, ifile) != 1)
		error ("cannot read auxiliary entry %d for symbol entry %d", j, k);
	    i++;
	}
    }
    cur_cmt = com_head;		/* initialize current comment */
}

static void
read_sections()
{
    int i;
    XCNHDR sh;		/* Section header structure */

    for (i = 0; i < num_sections; i++)
    {
	if (fseek (ifile, section_seek, 0) != 0)
	    error ("cannot seek to section headers");
	if (freads ((char *)&sh, sizeof (XCNHDR), 1, ifile) != 1)
	    error ("cannot read section headers");
#if !BIG_ENDIAN
	if (sh._n._s_n._s_zeroes)
	    swapw (sh._n._s_name, sizeof (long), 2);
#endif
	section_seek += sizeof (XCNHDR);

	dump_comments (i + 1);

	dump_data (&sh);
    }
}


static void
dump_comments (scnum)
int scnum;
{
    while ( cur_cmt && cur_cmt->c_scno <= scnum )
    {
        if ( cur_cmt->c_scno <= 0 )	/* no section mapping */
	    continue;

        eprintf( stdout, "_COMMENT\n" );

	if ( cur_cmt->c_text < sizeof ( str_length ) ||
	     cur_cmt->c_text > str_length )
	    error ( "invalid string table offset for comment" );

	eprintf( stdout, "%s\n",
	    &str_tab[ cur_cmt->c_text - sizeof( str_length ) ]);

	cur_cmt = cur_cmt->c_next;
    }
}


char *
get_secname (sh)
XCNHDR *sh;
{
    char *secname;

    if ( sh->_n._s_n._s_zeroes )
	secname = sh->_n._s_name;
    else
    {
	if (sh->_n._s_n._s_offset < sizeof (str_length) ||
	    sh->_n._s_n._s_offset > str_length)
	    error ("invalid string table offset for section header name");
	secname = &str_tab[sh->_n._s_n._s_offset - sizeof (str_length)];
    }

    return (secname);
}


static void
dump_data (sh)
XCNHDR *sh;
{
    char *secname, *get_secname ();
    long *raw_data;
    int  j;
    char emi_name_buf[8];

    if ( sh->_s.s_scnptr && sh->_s.s_size )
    {
	int memtype = CORE_ADDR_MAP( sh->_s.s_paddr );
	int address  = CORE_ADDR_ADDR( sh->_s.s_paddr );
	char *mem_field;
	
	secname = get_secname( sh );
	
	/* determine the memory field (optional counter ok) */
	switch ( memtype )
	{
	case memory_map_p:
	    mem_field = "P";
	    break;
	    
	case memory_map_pa:
	    mem_field = "PA";
	    break;
	    
	case memory_map_pb:
	    mem_field = "PB";
	    break;
	    
	case memory_map_pe:
	    mem_field = "PE";
	    break;
	    
	case memory_map_pi:
	    mem_field = "PI";
	    break;
	    
	case memory_map_pr:
	    mem_field = "PR";
	    break;
	    
	case memory_map_y:
	    mem_field = "Y";
	    break;
	    
	case memory_map_ya:
	    mem_field = "YA";
	    break;
	    
	case memory_map_yb:
	    mem_field = "YB";
	    break;
	    
	case memory_map_ye:
	    mem_field = "YE";
	    break;
	    
	case memory_map_yi:
	    mem_field = "YI";
	    break;
	    
	case memory_map_yr:
	    mem_field = "YR";
	    break;
	    
	case memory_map_x:
	    mem_field = "X";
	    break;
	    
	case memory_map_xa:
	    mem_field = "XA";
	    break;
	    
	case memory_map_xb:
	    mem_field = "XB";
	    break;
	    
	case memory_map_xe:
	    mem_field = "XE";
	    break;
	    
	case memory_map_xi:
	    mem_field = "XI";
	    break;
	    
	case memory_map_xr:
	    mem_field = "XR";
	    break;
	    
	case memory_map_l:
	    mem_field = "L";
	    break;
	    
	case memory_map_laa:
	    mem_field = "LAA";
	    break;
	    
	case memory_map_lab:
	    mem_field = "LAB";
	    break;
	    
	case memory_map_lba:
	    mem_field = "LBA";
	    break;
	    
	case memory_map_lbb:
	    mem_field = "LBB";
	    break;
	    
	case memory_map_le:
	    mem_field = "LE";
	    break;
	    
	case memory_map_li:
	    mem_field = "LI";
	    break;
	    
	case memory_map_emi:
	    mem_field = "EM";
	    break;
	    
	case memory_map_dm:
	    mem_field = "DM";
	    break;
	    
	default:

	    if ((file_header.f_magic == M566MAGIC) &&
		(memtype == memory_map_e1) && 
		(sh->_s.s_flags & STYP_OVERLAY) &&
		(!(sh->_s.s_flags & STYP_OVERLAYP)) 
		)
	    {
		/* substitute when x or y overlay memory destination */
		memtype = memory_map_e0;
	    }
	    
	    if (memtype>= memory_map_e0 && memtype<= memory_map_e255)
	{
	    sprintf(emi_name_buf,"E%d",memtype-memory_map_e0);
	    mem_field = emi_name_buf;
	}
	    else
	{
	    mem_field = "<ERROR>";
	}
	    break;
	}
	

	raw_data = (long *)malloc((size_t)(sh->_s.s_size * sizeof (long)));
	if (!raw_data)
	    error ("cannot allocate raw data for section %s", secname);

	if (fseek (ifile, sh->_s.s_scnptr, 0) != 0)
	    error ("cannot seek to raw data in section %s", secname);

	if (freads ((char *)raw_data, (int)sh->_s.s_size,
		    sizeof (long), ifile) != sizeof (long))
	    error ("cannot read raw data in section %s",  secname);


	/* check for block data */
	if ( sh->_s.s_flags & STYP_BLOCK )
	{
	    if ( mem_field[0] == 'L' )
	    {
		eprintf( stdout, "_BLOCKDATA Y %01.*X %01.*X %01.*X\n",
			addr_width, address,
			addr_width, CORE_ADDR_ADDR( sh->_s.s_vaddr ),
			data_width, *raw_data++ );

		eprintf( stdout, "_BLOCKDATA X %01.*X %01.*X %01.*X\n",
			addr_width, address,
			addr_width, CORE_ADDR_ADDR( sh->_s.s_vaddr ),
			data_width, *raw_data++ );
	    }
	    else
	    {
		eprintf( stdout, "_BLOCKDATA %s %01.*X %01.*X %01.*X\n",
			mem_field,
			addr_width, address,
			addr_width, CORE_ADDR_ADDR( sh->_s.s_vaddr ),
			data_width, *raw_data++ );
	    }
	}
	
	else
	{
	    eprintf (stdout, "_DATA %s %01.*X\n", mem_field,
		     addr_width, address );
	    
	    j = 0;
	    while ( j < sh->_s.s_size )
	    {
		if ( mem_field[0] == 'L' )
		{
		    eprintf (stdout, "%01.*lX %01.*lX ",
			     data_width, *(raw_data+1),
			     data_width, *raw_data);
		    raw_data += 2;
		    j += 2;
		}
		else
		{
		    eprintf (stdout, "%01.*lX ", data_width, *raw_data++);
		    j++;
		}
		

		if ( j % 8 == 0 && j < sh->_s.s_size )
		    eprintf (stdout, "\n");
	    }
	    eprintf (stdout, "\n");
	}
    }
}


static void
dump_deb_symbols ()
{
    SYMENT se;
    AUXENT ae;
    int    i, j, k;


    if (fseek (ifile, symptr, 0) != 0)
	error ("cannot seek to symbol table");

    i = 0;
    while (i < num_symbols)
    {
	if (freads ((char *)&se, sizeof (SYMENT), 1, ifile) != 1)
	    error ("cannot read symbol table entry %d", i);

	dump_se_d (&se); /* ek */
	k = i++;

	for (j = 0; j < se.n_numaux; j++)
	{
	    if (freads ((char *)&ae, sizeof (AUXENT), 1, ifile) != 1)
		error ("cannot read auxiliary entry %d for symbol entry %d", j, k);
	    i++;
	}
    }
}

static char *map_chars[]={
"P",
"X","Y","L","N",
"LAA","LAB","LBA","LBB","LE",
"LI","PA","PB","PE","PI",
"PR","XA","XB","XE","XI",
"XR","YA","YB","YE","YI",
"YR","PT","PF","EM",
"E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9 ",
"E10", "E11", "E12", "E13", "E14", "E15", "E16", "E17", "E18", "E19", 
"E20", "E21", "E22", "E23", "E24", "E25", "E26", "E27", "E28", "E29", 
"E30", "E31", "E32", "E33", "E34", "E35", "E36", "E37", "E38", "E39", 
"E40", "E41", "E42", "E43", "E44", "E45", "E46", "E47", "E48", "E49", 
"E50", "E51", "E52", "E53", "E54", "E55", "E56", "E57", "E58", "E59", 
"E60", "E61", "E62", "E63", "E64", "E65", "E66", "E67", 
"E68", "E69", "E70", "E71", "E72", "E73", "E74", "E75", 
"E76", "E77", "E78", "E79", "E80", "E81", "E82", "E83", 
"E84", "E85", "E86", "E87", "E88", "E89", "E90", "E91", 
"E92", "E93", "E94", "E95", "E96", "E97", "E98", "E99", 
"E100", "E101", "E102", "E103", "E104", "E105", "E106", "E107", 
"E108", "E109", "E110", "E111", "E112", "E113", "E114", "E115", 
"E116", "E117", "E118", "E119", "E120", "E121", "E122", "E123", 
"E124", "E125", "E126", "E127", "E128", "E129", "E130", "E131", 
"E132", "E133", "E134", "E135", "E136", "E137", "E138", "E139", 
"E140", "E141", "E142", "E143", "E144", "E145", "E146", "E147", 
"E148", "E149", "E150", "E151", "E152", "E153", "E154", "E155", 
"E156", "E157", "E158", "E159", "E160", "E161", "E162", "E163", 
"E164", "E165", "E166", "E167", "E168", "E169", "E170", "E171", 
"E172", "E173", "E174", "E175", "E176", "E177", "E178", "E179", 
"E180", "E181", "E182", "E183", "E184", "E185", "E186", "E187", 
"E188", "E189", "E190", "E191", "E192", "E193", "E194", "E195", 
"E196", "E197", "E198", "E199", "E200", "E201", "E202", "E203", 
"E204", "E205", "E206", "E207", "E208", "E209", "E210", "E211", 
"E212", "E213", "E214", "E215", "E216", "E217", "E218", "E219", 
"E220", "E221", "E222", "E223", "E224", "E225", "E226", "E227", 
"E228", "E229", "E230", "E231", "E232", "E233", "E234", "E235", 
"E236", "E237", "E238", "E239", "E240", "E241", "E242", "E243", 
"E244", "E245", "E246", "E247", "E248", "E249", "E250", "E251", 
"E252", "E253", "E254", "E255"
    };
static void
dump_se_d (se) /* for debug symbol table */
SYMENT *se;
{
    int  old_space;
#if 0
    char *type, *sclass;
#endif
    char *name;
    char sym_type = 'I';
	

    if (se->n_zeroes)
    {
#if !BIG_ENDIAN
	swapw (se->n_name, sizeof (long), 2);
#endif
	name = se->n_name;
    }
    else
    {
	if (se->n_offset < sizeof (str_length) ||
	    se->n_offset > str_length)
	    error ("invalid string table offset for symbol table entry %d name", se - symptr);
	name = &str_tab[se->n_offset - sizeof (str_length)];
    }

    if ( name[0] == '.' )
    {
	return;
    }


    if (ISFCN (se->n_type)); /* C functions - value type is that returned */
    else 
    {
	switch ( se->n_sclass )
	{
	case C_EFCN:
	case C_NULL:
	case C_AUTO:
	case C_EXT:
	case C_STAT:
	case C_REG:
	case C_EXTDEF:
	case C_LABEL:
	case C_ULABEL:
	case C_MOS:
	case C_ARG:
	case C_STRTAG:
	case C_MOU:
	case C_UNTAG:
	case C_TPDEF:
	case C_USTATIC:
	case C_ENTAG:
	case C_MOE:
	case C_REGPARM:
	case C_FIELD:
	case C_BLOCK:
	case C_FCN:
	case C_EOS:
	case C_FILE:
	case C_LINE:
	case C_ALIAS:
	case C_HIDDEN:
	case C_MEMREG:
	case C_OPTIMIZED:
	    break; /* use 'I' type for all the C types */
	default:
	    switch ( BTYPE( se->n_type ) )
	    {
	    case T_FLOAT:
	    case T_DOUBLE:
		sym_type = 'F';
		break;
#if 0
	    case T_STRUCT:
	    case T_UNION:
	    case T_ENUM:
	    case T_MOE:
	    case T_UCHAR:
	    case T_UINT:
	    case T_ULONG:
	    case T_NULL:
	    case T_CHAR:
	    case T_SHORT:
	    case T_INT:
	    case T_LONG:
	    default:
#endif
	    }
	}
    }

    /* ignore overhead symbols */
    if (strcmp (name, "etext") == 0 &&
	se->n_scnum == N_ABS &&
	se->n_type == T_NULL &&
	se->n_sclass == C_EXT)
	    return;
    if (strcmp (name, "end") == 0 &&
	se->n_scnum == N_ABS &&
	se->n_type == T_NULL &&
	se->n_sclass == C_EXT)
	    return;

    old_space = space;
    if (se->n_scnum > 0 || ISPTR (se->n_type))
	    space = CORE_ADDR_MAP ( se->n_value );
    else
	    space = memory_map_none;

    if (space < (sizeof (map_chars)/sizeof (char *))){
	if ( old_space != space ) eprintf( stdout, "_SYMBOL %s\n",map_chars[space] );
	/* print symbol name and value */
	eprintf (stdout, "%-19s  %c ", name, sym_type);
	if (sym_type == 'F') {
		union dbl val;
		val.l.hval = se->_n_value._n_val[1];
		val.l.lval = se->_n_value._n_val[0];
		eprintf (stdout, "%-.6E\n", val.dval);
	} else
		eprintf (stdout, "%01.*lX\n",
			 data_width, CORE_ADDR_ADDR (se->n_value));
	}
    else space = old_space; /* restore last written space */

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
freads (ptr, size, nitems, stream)
char *ptr;
int size, nitems;
FILE *stream;
{
    int rc;

    rc = fread (ptr, size, nitems, stream);
#if !BIG_ENDIAN
    swapw (ptr, size, nitems);
#endif
    return (rc);
}


#if !BIG_ENDIAN


union wrd
{   /* word union for byte swapping */
    unsigned long l;
    unsigned char b[4];
};

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
swapw (ptr, size, nitems)
char *ptr;
int size, nitems;
{
    union wrd *w;
    union wrd *end = (union wrd *)ptr +	((size * nitems) / sizeof (union wrd));
    unsigned i;

    for (w = (union wrd *)ptr; w < end; w++)
    {
	i = w->b[0];
	w->b[0] = w->b[3];
	w->b[3] = i;
	i = w->b[1];
	w->b[1] = w->b[2];
	w->b[2] = i;
    }
}
#endif


#if defined (LINT)
#ifdef va_dcl
#undef va_dcl
#define va_dcl char *va_alist;
#endif
#endif

/* VARARGS */

/* call fprintf, check for errors */
static void
#if defined ( __WATCOMC__ ) || defined( __DGUX__ ) || defined( MPW )
eprintf (FILE *fp, char *fmt, ...)
#else
eprintf( va_alist )
va_dcl
#endif
{
    void exit ();
    va_list ap;
#if defined ( __WATCOMC__ ) || defined( __DGUX__ ) || defined( MPW )
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
#if defined ( __WATCOMC__ ) || defined( __DGUX__ ) || defined( MPW )
error (char *fmt, ...)		/* display error on stderr, exit nonzero */
#else
error (va_alist)		/* display error on stderr, exit nonzero */
va_dcl
#endif
{
    void exit ();
    va_list ap;
#if !LINT
    int err = errno;
#endif
#if defined ( __WATCOMC__ ) || defined( __DGUX__ ) || defined( MPW )
    va_start (ap, fmt);
#else
    char *fmt;
    va_start (ap);
    fmt = va_arg (ap, char *);
#endif
    fprintf  (stderr, "cldlod: ");
    vfprintf (stderr, fmt, ap);
    fprintf  (stderr, "\n");
    va_end (ap);
#if !LINT
    if (err)
    {
	errno = err;
	perror ( "cldlod" );
    }
#endif
    exit (1);
}
