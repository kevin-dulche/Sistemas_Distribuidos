CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                                           C
C  Copyright (c) 1991-1997						    C
C  by ParallelTools, L.L.C. (PTOOLS), Houston, Texas			    C
C                                                                           C
C  This software is furnished under a license and may be used and copied    C
C  only in accordance with the terms of such license and with the	    C
C  inclusion of the above copyright notice.  This software or any other     C
C  copies thereof may not be provided or otherwise made available to any    C
C  other person.  No title to or ownership of the software is hereby	    C
C  transferred.                                                             C
C									    C
C  The recipient of this software (RECIPIENT) acknowledges and agrees that  C
C  the software contains information and trade secrets that are		    C
C  confidential and proprietary to PTOOLS.  RECIPIENT agrees to take all    C
C  reasonable steps to safeguard the software, and to prevent its	    C
C  disclosure.								    C 
C                                                                           C
C  The information in this software is subject to change without notice     C
C  and should not be construed as a commitment by PTOOLS.		    C
C                                                                           C
C  This software is furnished AS IS, without warranty of any kind, either   C
C  express or implied (including, but not limited to, any implied warranty  C
C  of merchantability or fitness), with regard to the software.  PTOOLS     C
C  assumes no responsibility for the use or reliability of its software.    C
C  PTOOLS shall not be liable for any special,	incidental, or		    C
C  consequential damages, or any damages whatsoever due to causes beyond    C
C  the reasonable control of PTOOLS, loss of use, data or profits, or from  C
C  loss or destruction of materials provided to PTOOLS by RECIPIENT.	    C
C									    C
C  PTOOLS's liability for damages arising out of or in connection with the  C
C  use or performance of this software, whether in an action of contract    C
C  or tort including negligence, shall be limited to the purchase price,    C
C  or the total amount paid by RECIPIENT, whichever is less.		    C
C                                                                           C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C
C $Id: Tmk_fortran.h,v 10.3 1997/06/01 08:17:45 alc Exp $
C
C Description:    
C	user include file for FORTRAN
C
C Facility:	TreadMarks Distributed Shared Memory System
C History:
C	11-Jul-1994	Sandhya Dwarkadas 	Created
C	21-May-1996	Rob Fowler		Converted to pure FORTRAN
C
      integer Tmk_proc_id
      integer Tmk_nprocs

      integer Tmk_page_size
      integer Tmk_npages

      common /Tmk/ Tmk_proc_id, Tmk_nprocs, Tmk_page_size, Tmk_npages
