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
 # $Id: Makefile,v 1.2 1998/09/11 06:28:10 alc Exp $
 #
 # Description:    
 #	builds the ATM and Ethernet libraries
 #
 # Facility:	TreadMarks Distributed Shared Memory System
 # History:
 #	19-Apr-1996	Robert J. Fowler
 #					Created
 #	Version 0.10.1
 #

CC	= gcc
CFLAGS	= -g -m32 -O2 -I../include

ROOT	= ..

MACHDEP_OBJS=

all: libTmk_udp.a

include $(ROOT)/.common
