/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: CAPI tracer defines
 *      Author: oliver
 *    $RCSfile: capidump.h,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: include file for CAPi tracer
 *    Products: capitrace	(remote application for UNIX-Hosts)
 *    		DimeTools	(remote Windows application)
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/

#ifndef _CAPIDUMP_H
#define _CAPIDUMP_H

#ifdef __cplusplus
    extern "C" {
#endif

/* CAPI trace flag values */

#define FL_LONGOUT	1l
#define FL_SHORTOUT	2l
#define FL_HEXOUT	4l
	
/* function prototypes */

#ifndef PROTO	
#ifdef __STDC__
#define PROTO(x)	x
#else
#define PROTO(x)	()
#endif
#endif
	
/*
 *	TYPE'S and STRUCT'S
 */

/* type declaration for CAPI messages not in capidef.h */
    typedef struct capitrace_s	capitrace_t;
    struct capitrace_s {
	unsigned long 		timestamp;
	unsigned long 		inout;
	unsigned long		type;
	unsigned long		seqcnt;
    };


/*
 *	PROTO'S
 */
int	dump_capimsg PROTO(( union CAPI_primitives *, unsigned long,
				unsigned long, unsigned long, unsigned long));

#ifdef __cplusplus
    }
#endif

#endif
