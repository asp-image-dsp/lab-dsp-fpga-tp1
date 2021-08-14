# $Id: makefile,v 1.33 1998/01/09 23:13:23 jay Exp $
CC	=gcc  -static	# GNU compiler
#CC	=cc	# NeXT,DEC,Apollo, Solaris


# HOSTNAME hp,sun4,next,apl,dec,solaris
H	= solaris

ifeq ($(H),hp)
CC	=cc -Aa -D_HPUX_SOURCE 
endif

# OBJECT DIRECTORY
O	= obj/$(H)

OS	= -DUNIX	# Sun, DEC, Apollo, HP ,Solaris

ENDIAN	= -DBIG_ENDIAN=1	# everything but DEC and PC

INSTALL		= /bin/mv
INSTALLDIR	= 		#

CLASDIR=clas566
#CLASDIR=clas96
#CLASDIR=clas16

EXE=

TXTCPY = cp
ifeq ($(H),nt)
TXTCPY = unix2dos
EXE=.exe
endif
ifeq ($(H),wpc)
TXTCPY = unix2dos
EXE=.exe
endif

DROOT=../..
#DROOT=
DIST=dist
DISTDIR=$(DROOT)/$(DIST)/$(H)/$(CLASDIR)
DBIN=$(DISTDIR)/bin
DTOOL=$(DISTDIR)/cldtools
DISTSRC=cldinfo.c cldlod.c cofdmp.c srec.c strip.c tiohist.c cofdmp.h srec.h strip.h makefile
DISTBIN= $(O)/cldinfo$(EXE) $(O)/cldlod$(EXE) $(O)/cofdmp$(EXE) $(O)/srec$(EXE) $(O)/strip$(EXE) $(O)/tiohist$(EXE) 
DISTTXT= cldinfo.txt cldlod.txt cofdmp.txt srec.txt strip.txt tiohist.txt

info:
	@echo 'for the following commands, xxx=sun4|hp|solaris|next|apl|dec'
	@echo 'use "make H=xxx all"'
	@echo 'then "make H=xxx alltests"'
	@echo 'then "make H=xxx CLASDIR=clasnn DIST=dist|alpha dist|alpha", nn=16|56|96'
	@echo 'then "make H=xxx clean"'


all: obdir $(O)/cldinfo $(O)/cldlod $(O)/srec $(O)/cofdmp $(O)/strip $(O)/tiohist

obdir: 
	test -d obj || mkdir obj
	test -d $(O) || mkdir $(O)

$(DIST):
	test -d $(DROOT)/$(DIST) || mkdir $(DROOT)/$(DIST)
	test -d $(DROOT)/$(DIST)/$(H) || mkdir $(DROOT)/$(DIST)/$(H)
	test -d $(DISTDIR) || mkdir $(DISTDIR)
	test -d $(DBIN) || mkdir $(DBIN)
	test -d $(DTOOL) || mkdir $(DTOOL)
	for i in ${DISTBIN} ; do cp $$i $(DBIN)/. ; done
	for i in ${DISTTXT} ; do $(TXTCPY) $$i $(DBIN)/$$i ; done
	for i in ${DISTSRC} ; do $(TXTCPY) $$i $(DTOOL)/$$i ; done

clean: 
	test -d tests/$(H) && cd tests/$(H);make clean
	rm -rf $(O)

testclean: 
	test -d tests/$(H) && cd tests/$(H);make clean

$(O)/cldlod: cldlod.c
	$(CC) -O $(OS) $(ENDIAN) -I../mcoffinc cldlod.c -o $(O)/cldlod

$(O)/cldinfo: cldinfo.c
	$(CC) -O $(OS) $(ENDIAN) -I../mcoffinc cldinfo.c -o $(O)/cldinfo

$(O)/srec: srec.c
	$(CC) -O $(OS) $(ENDIAN) -I../mcoffinc srec.c -o $(O)/srec

$(O)/cofdmp: cofdmp.c
	$(CC) -O $(OS) $(ENDIAN) -I../mcoffinc cofdmp.c -o $(O)/cofdmp
# Following should be included only for disassembly option
#	/usr1/dsp/software/dist/$H/clas56/sim/cm56000.a \
#	/usr1/dsp/software/dist/$H/clas96/sim/cm96000.a \
#	/usr1/dsp/software/dist/$H/clas16/sim/cm56100.a \
#	/usr1/dsp/software/alpha/$H/clas563/sim/cm56300.a \
#	/usr1/dsp/software/alpha/$H/clas568/sim/cm56800.a

$(O)/strip: strip.c
	$(CC) -O $(OS) $(ENDIAN) -I../mcoffinc strip.c -o $(O)/strip

$(O)/tiohist: tiohist.c
	$(CC) -O $(OS) $(ENDIAN) tiohist.c -o $(O)/tiohist

install:
	$(INSTALL) $(DISTBIN) $(INSTALLDIR)

alltests:
	cd tests; make tests.$(H)
