/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: hexdump.c,v $
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

#include "capidef.h"
#include "libcapi.h"

/****************************************************************************
 *
 *	capi_fhexdump()
 *
 ****************************************************************************/
int capi_fhexdump(FILE		*fp,
		  char		*buf,
		  size_t	len,
		  size_t	lineWidth,
		  size_t	lineOffset)
{
    size_t		i, k;
    unsigned char	*data = (unsigned char*)buf;
    unsigned char	c;
    size_t		ib;
    unsigned char	ascbuf[0x20];

    lineWidth = lineWidth<sizeof(ascbuf) ? lineWidth : sizeof(ascbuf)-1;

    for (i=0; i<len; i+=lineWidth) {
	if (!(i%lineWidth)) {
	    fprintf( fp, "%*s%04x: ", (int)lineOffset, "", (int)i);
	}

	for (ib=0,k=i; k < i+lineWidth; k++) {
	    if (k == i+(lineWidth/2)) fprintf( fp, " ");
	    c = data[k];
	    if (k < len) {
		fprintf( fp, "%02x ", c & 0xff);
		ascbuf[ib++] = (c >= 0x20 && c <127) ? c : '.';
	    } else {
		fprintf( fp, "   ");
	    }
	}
	ascbuf[ib] = 0;
	fprintf( fp, " %s\n", ascbuf);
    }
    return(0);
}

/****************************************************************************
 *
 *	capi_hexdump()
 *
 ****************************************************************************/
int capi_hexdump(char	*buf,
		 size_t	len,
		 size_t	lineWidth,
		 size_t	lineOffset)
{
    return capi_fhexdump(stdout, buf, len, lineWidth, lineOffset);
}

