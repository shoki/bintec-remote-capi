/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: c_blockrw.c,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: streams driver/module | application | library
 *    Products: ALL | XS,XM,XL,XP,BGO,BGP,XCM
 *  Interfaces: IPI, DLPI
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>

#include "capidef.h"
#include "libcapi.h"
#include <unistd.h>

#if	1	/* print error */
    static int c_blockrw_tracelevel         = 1;
#   define cTRACE(l, c)             if ((l) <= c_blockrw_tracelevel) { c; }
#else
#   define cTRACE(l, c)             /* do nothing */
#endif


/****************************************************************************
 *
 * 	capi_blockread()
 *
 ****************************************************************************/
size_t capi_blockread(int 	fd, 
		      VOIDPTR 	block, 
		      size_t 	size)
{
    char		*ptr	= block;
    unsigned long	len;

    while (size > 0) {
	len = read( fd, ptr, (size_t)size);
	if(len > 0){		
	    size -= len;
	    ptr  += len;
	    continue;
	}
	else if(len == 0){	/* got a EOF */
	    cTRACE( 1, fprintf( stderr, "capi_blockread <<EOF>>\n" ));
	    break;
	}
	/* if((len < 0) && (errno == EINTR)) */
	if(errno == EINTR){	
	    continue;
	}
	/* errno is only set if systemcall returns -1 */
	cTRACE( 1, perror("capi_blockread"));
	/* this is wrong */
	cTRACE( 1, fprintf( stderr, "capi_blockread len=%ld size=%d waitfor=%lu\n",
		len, ptr-(char*)block, (unsigned long)size));
	return( -1 );
    }
    return (ptr-(char*)block);
}
/****************************************************************************
 *
 *	capi_blockwrite()
 *
 ****************************************************************************/
size_t capi_blockwrite(int 	fd, 
		       VOIDPTR 	block, 
		       size_t 	size)
{
    char           *ptr = block;
    unsigned long  len;
    fd_set         fds;
    struct timeval tv;

    while (size > 0) {
	do {
again:
	    FD_ZERO(&fds);
	    FD_SET(fd, &fds);
	    tv.tv_sec = 10;
	    tv.tv_usec = 0;
	    switch(select(fd+1, NULL, &fds, NULL, &tv)) {
		case  0:
		    cTRACE(0, fprintf(stderr, "select: timeout\n"));
		    return -1;
		case -1:
		    cTRACE( 0, perror("select"));
		    if(errno == EINTR) goto again;
		    return -1;
	    }
	    errno = 0;
	    len = write( fd, ptr, size);
	    /*
	     *  if (errno == EINTR) cTRACE( 0, perror("write:EINTR"));
	     */
	} while((len <= 0) && (errno == EINTR));
	if (len > 0 ) {
	    size -= len;
	    ptr  += len;
	    continue;
	}
	if (len <= 0) {
	    cTRACE( 0, perror("blockwrite"));
	    if(len==0){
		cTRACE( 0, fprintf( stderr, "blockwrite <<EOF>>\n"));
	    }
	    else {
		/* again wrong! */
		cTRACE( 0, fprintf( stderr, 
				    "blockwrite len=%ld size=%d waitfor=%lu\n",
				    len, ptr-(char*)block, (unsigned long)size));
	    }
	    return -1;
	}
    }
    return (ptr-(char*)block);
}

