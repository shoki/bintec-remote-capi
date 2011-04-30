/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: c_environ.c,v $
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

#include "capiconf.h"

/****************************************************************************
 *   This implementation includes the following function calls:

int  capi_read_environ( char **ppHost, int *pPort);

 ****************************************************************************/

#if	1	/* print error */
    static int c_eviron_tracelevel         = 1;
#   define cTRACE(l, c)             if ((l) <= c_eviron_tracelevel) { c; }
#else
#   define cTRACE(l, c)             /* do nothing */
#endif


/****************************************************************************
 *
 *	capi_read_environ()
 *
 ****************************************************************************/
int capi_read_environ(char	**ppHost,
		      int	*pPort)
{
    char	*cp;
    char	*ep;
    int 	port	= CAPI_PORT_DEFAULT;
    
    /*
     * is CAPI_PORT_ENV available ???
     */
    if( pPort ) {
	*pPort = port;
	if ((cp = getenv(CAPI_PORT_ENV)) != NULL) {
	    port = strtol( cp, &ep, 0);
	    if (*ep) goto error;
	    *pPort = port;
	}
    }

    /*
     * is CAPI_HOST_ENV available ???
     */
    if( ppHost ) {
	if ((cp = getenv(CAPI_HOST_ENV)) == NULL) {
	    cTRACE( 1, fprintf( stderr,
			"libcapi: environment variable CAPI_HOST missing!\n"));
	    goto error;
	}
	*ppHost	= cp;
    }
    return(0);

error:
    return(-1);
}


