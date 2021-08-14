/* $Id: cldinfo.c,v 1.26 1998/01/07 19:39:48 jay Exp $ */
/*
  to make for most systems,

  cc -O -DBIG_ENDIAN=1 -I. cldinfo.c

  for PCs and DEC3100,

  cc -O -I. cldinfo.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0
#endif
#include "coreaddr.h" 
#include "maout.h"


/* we use this flag to tell whether or not we're working on a .cld file. */
static int not_a_cld_p;

static
struct{ /* used for loading section data */
	long block_size;
	long *mem;
} raw_data;

static long psize = 0l, xsize = 0l, ysize = 0l, start_address = 0l, emsize = 0L, dmsize = 0L, usize = 0L;

/* Routines for reading headers and symbols from executable.  */

#if !BIG_ENDIAN
union ulong{
	unsigned long ul;
	unsigned char uc[4];
};

static void
swapem(uptr,usiz)
union ulong *uptr;
unsigned long usiz;
{
	unsigned long i;
	unsigned char uc0,uc1,*ucp;
	usiz/= sizeof(long);

	for (i=0;i<usiz;i++){
		ucp= &uptr[i].uc[0];
		uc0=ucp[0];
		uc1=ucp[1];
		ucp[0]=ucp[3];
		ucp[1]=ucp[2];
		ucp[2]=uc1;
		ucp[3]=uc0;
	}
}
#endif



/* Read COFF file header, check magic number,
   and return number of symbols. */
static int
read_file_hdr (chan, fil_hdr)
FILE * chan;
FILHDR *fil_hdr;
{
	   (void) fseek (chan, 0L, 0);

	if (fread ((char *)fil_hdr, FILHSZ, 1,chan) != 1)
	{
	    perror( "read_file_hdr" );
	    exit ( -2 );
	}
	
#if !BIG_ENDIAN
	swapem((union ulong *)fil_hdr,(unsigned long)FILHSZ);
#endif

	switch (fil_hdr->f_magic)
	{
#ifdef M566MAGIC
	case M566MAGIC:
#endif            
#ifdef M568MAGIC
	case M568MAGIC:
#endif
#ifdef M563MAGIC
	case M563MAGIC:
#endif
#ifdef M16KMAGIC
	case M16KMAGIC:
#endif
#ifdef M96KMAGIC
	case M96KMAGIC:
#endif
#ifdef M56KMAGIC
	case M56KMAGIC:
#endif
#ifdef M569MAGIC
	case M569MAGIC:
#endif
	    return fil_hdr->f_nsyms;

	default:
#ifdef BADMAG
	    if (BADMAG(fil_hdr))
	    {
		fprintf( stderr, "read_file_hdr() failure\n" );
		exit ( -2 );
	    }
	    
	    else
	    {
		return fil_hdr->f_nsyms;
	    }
	    
#else
	    fprintf( stderr, "read_file_hdr() failure\n" );
	    exit ( -2 );
#endif
	}
	return (0);
}

static int
read_aout_hdr (chan, ao_hdr, size)
FILE *chan;
AOUTHDR *ao_hdr;
int size;

{
	   (void) fseek (chan, (long)FILHSZ, 0);

    if (size != sizeof (AOUTHDR))
    {
	fprintf( stderr, "read_aout_hdr() failure\n" ) ;
	exit ( -3 );
    }
    

    if (fread ((char *)ao_hdr, size,1,chan) != 1)
    {
	perror( "read_aout_hdr" );
	exit( -3 );
    }
    
#if !BIG_ENDIAN
    swapem((union ulong *)ao_hdr,(unsigned long)size);
#endif

    return 0;
}

static void
read_section_contents( FILE *chan )
{
    FILHDR fl_hdr;
    AOUTHDR ao_hdr;
    SCNHDR section_hdr;
    int i;
    enum memory_map memtype;
    unsigned long section_length;
    
    read_file_hdr ( chan, & fl_hdr );
    
    if ( fl_hdr.f_opthdr != sizeof ( AOUTHDR ))
    {
	/* this isn't a .cld file. make a note, and continue on. */
	not_a_cld_p = 1;
    }
    else
    {
	read_aout_hdr ( chan, & ao_hdr, (int) sizeof ( AOUTHDR ));
    }
    
    for ( i = 0; i < (int) fl_hdr.f_nscns; ++ i )
    {
	if ( fseek ( chan, 
		    (long)(FILHSZ + fl_hdr.f_opthdr + (i * SCNHSZ)) , 0 )
	    != 0 )
	{
	    perror( "read_section_contents 1" );
	    exit( -5 );
	}
	
	
	if ( fread ( (char *) & section_hdr, SCNHSZ,1,chan ) != 1 )
	{
	    perror( "read_section_contents 2" );
	    exit( -5 );
	}
	
#if !BIG_ENDIAN
	swapem ((union ulong *) & section_hdr, (unsigned long) SCNHSZ );
	swapem ((union ulong *) section_hdr.s_name, 8l );
#endif
	
	/* we want to summarize all of the sections *except* for ".text"
	   and ".data"; these themselves are summaries of a sort that are
	   needed by the debugger. including them would incorrectly count
	   all object twice (and then some). */
	if ( strncmp ( section_hdr.s_name, ".text", 8 ) &&
	    strncmp ( section_hdr.s_name, ".data", 8 ))
	{
	    /* load memory block */
	    if ( section_hdr.s_size > raw_data.block_size )
	    {
		if ( raw_data.mem )
		{
		    free ( raw_data.mem );
		}
		if (!(raw_data.mem=
		      (long *) malloc ((size_t)( ( section_hdr.s_size )
					 * sizeof ( long )))))
		{
		    perror( "malloc error" );
		    exit( -5 );
		}

		raw_data.block_size = section_hdr.s_size;
	    }
	    
	    if ( fseek ( chan, section_hdr.s_scnptr, 0 ) != 0 )
	    {
		perror( "read_section_contents 3" );
		exit( -5 );
	    }
	    
	    if ( section_hdr.s_size && (fread ( (char *) raw_data.mem,
		       (int)( section_hdr.s_size * sizeof(long)),1,chan) != 1 ))
	    {
		perror( "read_section_contents 4" );
		exit( -5 );
	    }
	    
#if !BIG_ENDIAN
	    swapem ((union ulong *) raw_data.mem,
		    (unsigned long) ( section_hdr.s_size * sizeof(long)));
#endif
	    switch ( memtype = CORE_ADDR_MAP ( section_hdr.s_paddr ))
	    {
	    case memory_map_pa:
	    case memory_map_pb:
	    case memory_map_pe:
	    case memory_map_pi:
	    case memory_map_pr:
	    case memory_map_p8:
		memtype = memory_map_p;
		break;
		
	    case memory_map_xa:
	    case memory_map_xb:
	    case memory_map_xe:
	    case memory_map_xi:
	    case memory_map_xr:
		memtype = memory_map_x;
		break;
		
	    case memory_map_ya:
	    case memory_map_yb:
	    case memory_map_ye:
	    case memory_map_yi:
	    case memory_map_yr:
		memtype = memory_map_y;
		break;
		
	    case memory_map_laa:
	    case memory_map_li:
	    case memory_map_lab:
	    case memory_map_lba:
	    case memory_map_lbb:
	    case memory_map_le:
		memtype = memory_map_l;
		break;

	    case memory_map_u8:
	    case memory_map_u16:
		memtype = memory_map_u;
		break;
	    default:
		break;
	    }
	    
	    if ( section_hdr.s_flags & STYP_BLOCK )
	    {
		section_length = CORE_ADDR_ADDR ( section_hdr.s_vaddr );
		
		if ( memtype == memory_map_l )
		{
		    xsize += section_length;
		    ysize += section_length;
		}
		else if ( memtype == memory_map_y )
		{
		    ysize += section_length;
		}
		else if ( memtype == memory_map_p )
		{
		    psize += section_length;
		}
		else if ( memtype == memory_map_x )
		{
		    xsize += section_length;
		}
		else if ( memtype == memory_map_dm )
		{
		    dmsize += section_length;
		}
		else if ( memtype>= memory_map_emi && memtype<= memory_map_e255 )
		{
		    emsize += section_length;
		}
		else if ( memtype == memory_map_u )
		{
		    usize += section_length;
		}
	    }
	    else
	    {
		section_length = section_hdr.s_size;
		if ( memtype == memory_map_l )
		{
		    section_length >>= 1;
		    xsize += section_length;
		    ysize += section_length;
		}
		else if ( memtype == memory_map_y )
		{
		    ysize += section_length;
		}
		else if ( memtype == memory_map_p )
		{
		    psize += section_length;
		}
		else if ( memtype == memory_map_x )
		{
		    xsize += section_length;
		}
		else if ( memtype == memory_map_dm )
		{
		    dmsize += section_length;
		}
		else if ( memtype>= memory_map_emi && memtype<= memory_map_e255 )
		{
		    emsize += section_length;
		}
		else if ( memtype == memory_map_u )
		{
		    usize += section_length;
		}
	    }
	}
    }
    
    if ( ! not_a_cld_p )
    {
	start_address = CORE_ADDR_ADDR ( ao_hdr.entry );
    }
    
    if ( raw_data.mem )
    {
	free ( raw_data.mem );
	raw_data.block_size = 0;
	raw_data.mem = 0;
    }
}


static void
cld_info ( execchan,xs,ys,ps,sa,ems,dms,us )
    FILE *execchan;
    long *xs,*ys,*ps,*sa,*ems,*dms,*us;
{
    /* Now open and digest the file the user requested, if any.  */
    read_section_contents( execchan );
    
    *xs = xsize; 
    *ps = psize; 
    *ys = ysize; 
    *ems = emsize;
    *dms = dmsize;
    *us = usize;
    if ( ! not_a_cld_p )
    {
	*sa = start_address;
    }
}


void
main(argc,argv)
int argc; 
char **argv;
{
    long xs,ys,ps,sa,ems,dms,us;
    FILE *ifile ;	/* file pointer for input file */
    ifile = NULL;

    if ( argc != 2 )
    {
	fprintf( stderr, "Version 6.2 usage: cldinfo file\n" );
	exit (-1 );
    }
    if (( ifile = fopen (argv[1], "rb" )) == NULL )
    {
	perror( "cldinfo" );
	exit( -2 );
    }
    
    cld_info (ifile, & xs, & ys, & ps, & sa, & ems, & dms, & us );

    (void) fclose ( ifile );
    
    fprintf ( stdout,"filename: %s\n", argv[1] );

    if ( not_a_cld_p )
    {
	fprintf ( stdout,
		 "\txsize: %ld, ysize: %ld, psize: %ld, emsize: %ld, dmsize: %ld, usize: %ld\n",
		 xs, ys, ps, ems, dms, us );
    }
    else
    {
	fprintf ( stdout,
		 "\txsize: %ld, ysize: %ld, psize: %ld, emsize: %ld, dmsize: %ld, usize: %ld, start addr: %lx\n",
		 xs, ys, ps, ems, dms, us, sa );
    }
    exit ( 0 );
}
