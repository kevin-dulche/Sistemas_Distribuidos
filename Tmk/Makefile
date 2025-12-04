 #############################################################################
 #                                                                           #
 #  Copyright (c) 1991-1994                                                  #
 #  by TreadMarks, L.L.C. (TREADMARKS), Houston, Texas	  		     #
 #                                                                           #
 #  This software is furnished under a license and may be used and  copied   #
 #  only  in  accordance  with  the  terms  of  such  license and with the   #
 #  inclusion of the above copyright notice.  This software or  any  other   #
 #  copies  thereof may not be provided or otherwise made available to any   #
 #  other person.  No title to or ownership of  the  software  is  hereby    #
 #  transferred.                                                             #
 #									     #
 #  The recipient of this software (RECIPIENT) acknowledges and agrees that  #
 #  the software contains information and trade secrets that are	     #
 #  confidential and proprietary to TREADMARKS.  RECIPIENT agrees to take    #
 #  all reasonable steps to safeguard the software, and to prevent its       #
 #  disclosure.								     # 
 #                                                                           #
 #  The information in this software is subject to change  without  notice   #
 #  and should  not  be  construed  as  a commitment by TREADMARKS.	     #
 #                                                                           #
 #  This software is furnished AS IS, without warranty of any kind, either   #
 #  express or implied (including, but not limited to, any implied warranty  #
 #  of merchantability or fitness), with regard to the software.  	     #
 #  TREADMARKS assumes no responsibility for the use or reliability of its   #
 #  software.  TREADMARKS shall not be liable for any special, incidental,   #
 #  or consequential damages, or any damages whatsoever due to causes beyond #
 #  the reasonable control of TREADMARKS, loss of use, data or profits, or   #
 #  from loss or destruction of materials provided to TREADMARKS by	     #
 #  RECIPIENT.								     #
 #									     #
 #  TREADMARKS's liability for damages arising out of or in connection with  #
 #  the use or performance of this software, whether in an action of	     #
 #  contract or tort including negligence, shall be limited to the purchase  #
 #  price, or the total amount paid by RECIPIENT, whichever is less.	     #
 #                                                                           #
 #############################################################################
 
 #
 # $Id: Makefile,v 10.11 1998/11/04 05:45:50 alc Exp $
 #
 # Description:    
 #	builds the ATM and Ethernet libraries and the sample programs
 #
 # Facility:	TreadMarks Distributed Shared Memory System
 #
 # History:
 #	 7-Sep-1993	Pete Keleher	Created
 #	15-May-1994	Alan L. Cox	Added support for the RS/6000
 #	14-Jun-1994	Alan L. Cox	Added support for the Alpha
 #					  (changes provided by Povl Koch)
 #	Version 0.9.0
 #
 #	Version 0.9.1
 #
 #	18-Jan-1995     Rob Fowler	Demo support
 #
 #	Version 0.9.2
 #
 #	 4-Apr-1995	Alan L. Cox	Added support for SGI/IRIX
 #	10-Apr-1995	Alan L. Cox	Added support for the HP PA
 #      28-Apr-1995     Rob Fowler	Automation of binary distributions
 #	29-Apr-1995	Alan L. Cox	Added support for Solaris
 #	25-May-1995	Alan L. Cox	Defined SHELL for SGI/IRIX
 #
 #	Version 0.9.3
 #
 #	15-Jan-1996	Alan L. Cox	Adapted for FreeBSD 2.1/Intel x86
 #
 #	Version 0.9.7
 #

SHELL	= /bin/sh

all:
	@echo 'Usage: make alpha_{linux2,osf4}"|"hppa_hpux10"|"i386_{bsdi3,freebsd2,linux2,sunos5}"|"mips_irix6"|"rs6k_{aix4,mpl3}"|"sparc_{sunos4,sunos5}'

all_src:
	path=`pwd | sed 's-.*/--'` ; \
	cd .. ; \
	tar cfv $$path/$$path.tar $$path/Makefile $$path/RCS $$path/ReleaseNotes* $$path/.common $$path/apps $$path/doc $$path/include $$path/lib.* $$path/src

clean: clean_lib clean_apps

clean_lib:
	for path in lib.* ; do cd $$path ; $(MAKE) clean ; cd .. ; done

clean_apps:
	cd apps ; \
	for path in * ; do \
		cd $$path ; \
		for arch in bin.* ; do cd $$arch ; $(MAKE) clean ; cd .. ; done ; \
		cd .. ; \
	done

alpha_linux2_bin alpha_osf4_bin hppa_hpux10_bin i386_bsdi3_bin i386_freebsd2_bin i386_linux2_bin i386_sunos5_bin mips_irix6_bin rs6k_aix4_bin sparc_sunos4_bin sparc_sunos5_bin:
	path=`pwd | sed 's-.*/--'` ; \
	cd .. ; \
	tar cfv $$path/$$path-$(@:_bin=).tar $$path/ReleaseNotes* $$path/doc $$path/include $$path/lib.$(@:_bin=)/libTmk_udp.a $$path/lib.$(@:_bin=)/support $$path/apps/Jacobi_f/src $$path/apps/Jacobi_f/.common $$path/apps/Jacobi_f/bin.$(@:_bin=) $$path/apps/sor_c/README $$path/apps/sor_c/src $$path/apps/sor_c/.common $$path/apps/sor_c/bin.$(@:_bin=) $$path/apps/test_c/README $$path/apps/test_c/src $$path/apps/test_c/.common $$path/apps/test_c/bin.$(@:_bin=) $$path/apps/tsp_c/README $$path/apps/tsp_c/src $$path/apps/tsp_c/.common $$path/apps/tsp_c/bin.$(@:_bin=) $$path/apps/tsp_c/tspfiles ; \
	cd $$path ; \
	compress -f $$path-$(@:_bin=).tar ; \
	uuencode $$path-$(@:_bin=).tar.Z $$path-$(@:_bin=).tar.Z > $$path-$(@:_bin=).tar.Z.uu

alpha_linux2_bin: alpha_linux2
alpha_osf4_bin: alpha_osf4
hppa_hpux10_bin: hppa_hpux10
i386_bsdi3_bin: i386_bsdi3
i386_freebsd2_bin: i386_freebsd2
i386_linux2_bin: i386_linux2
i386_sunos5_bin: i386_sunos5
mips_irix6_bin: mips_irix6
rs6k_aix4_bin: rs6k_aix4
sparc_sunos4_bin: sparc_sunos4
sparc_sunos5_bin: sparc_sunos5

rs6k_mpl3_bin: rs6k_mpl3
	path=`pwd | sed 's-.*/--'` ; \
	cd .. ; \
	tar cfv $$path/$$path-$(@:_bin=).tar $$path/ReleaseNotes* $$path/doc $$path/include $$path/lib.$(@:_bin=)/libTmk_mpl.a $$path/lib.$(@:_bin=)/support $$path/apps/Jacobi_f/src $$path/apps/Jacobi_f/.common $$path/apps/Jacobi_f/bin.$(@:_bin=) $$path/apps/sor_c/README $$path/apps/sor_c/src $$path/apps/sor_c/.common $$path/apps/sor_c/bin.$(@:_bin=) $$path/apps/test_c/README $$path/apps/test_c/src $$path/apps/test_c/.common $$path/apps/test_c/bin.$(@:_bin=) $$path/apps/tsp_c/README $$path/apps/tsp_c/src $$path/apps/tsp_c/.common $$path/apps/tsp_c/bin.$(@:_bin=) $$path/apps/tsp_c/tspfiles ; \
	cd $$path ; \
	compress -f $$path-$(@:_bin=).tar ; \
	uuencode $$path-$(@:_bin=).tar.Z $$path-$(@:_bin=).tar.Z > $$path-$(@:_bin=).tar.Z.uu

alpha_linux2 alpha_osf4 hppa_hpux10 i386_bsdi3 i386_freebsd2 i386_linux2 i386_sunos5 mips_irix6 rs6k_aix4 rs6k_mpl3 sparc_sunos4 sparc_sunos5:
	cd lib.$@ ; $(MAKE)
	cd apps ; for path in * ; do cd $$path/bin.$@ ; $(MAKE) ; cd ../.. ; done

rs6k_aix4_ll_bin: rs6k_aix4_ll
	path=`pwd | sed 's-.*/--'` ; \
	cd .. ; \
	tar cfv $$path/$$path-$(@:_bin=).tar $$path/ReleaseNotes* $$path/doc $$path/include $$path/lib.$(@:_ll_bin=)/libTmk_udp.a $$path/lib.$(@:_ll_bin=)/libTmk_udp_ll.a $$path/lib.$(@:_ll_bin=)/support $$path/apps/Jacobi_f/README $$path/apps/Jacobi_f/src $$path/apps/Jacobi_f/.common $$path/apps/Jacobi_f/bin.$(@:_bin=) $$path/apps/sor_c/README $$path/apps/sor_c/src $$path/apps/sor_c/.common $$path/apps/sor_c/bin.$(@:_bin=) $$path/apps/test_c/README $$path/apps/test_c/src $$path/apps/test_c/.common $$path/apps/test_c/bin.$(@:_bin=) $$path/apps/tsp_c/README $$path/apps/tsp_c/src $$path/apps/tsp_c/.common $$path/apps/tsp_c/bin.$(@:_bin=) $$path/apps/tsp_c/tspfiles ; \
	cd $$path ; \
	compress -f $$path-$(@:_bin=).tar ; \
	uuencode $$path-$(@:_bin=).tar.Z $$path-$(@:_bin=).tar.Z > $$path-$(@:_bin=).tar.Z.uu

rs6k_aix4_ll:
	cd lib.$(@:_ll=) ; $(MAKE) ; $(MAKE) libTmk_udp_ll.a
	cd apps ; for path in * ; do cd $$path/bin.$(@:_ll=) ; $(MAKE) ; cd ../.. ; done
