/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: c_tcpopen.c,v $
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

#ifndef INADDR_NONE
#   define INADDR_NONE     0xffffffff      /* should be in <netinet/in.h> */
#endif

/****************************************************************************
 *   This implementation includes the following function calls:
 *
 *
 * int  capi_tcp_open    ( char *host, char *service, int port);
 *
 ****************************************************************************/

#if	1	/* print error */
    static int librcapi_tracelevel         = 1;
#   define cTRACE(l, c)             if ((l) <= librcapi_tracelevel) { c; }
#else
#   define cTRACE(l, c)             /* do nothing */
#endif


/***********************************************************************
 *
 *	capi_tcp_open()
 * name or dotted-decimal addr of other system
 * name of service being requested                     *
 * if == 0, nothing special - use port# of service 
 * if > 0, it's the port# of server (host-byte-order) 
 * name or dotted-decimal addr of other system 
 * if < 0, bind a local reserved port
 *
 **********************************************************************/



int capi_tcp_open(char    *host,
		  char    *service,
		  int     port)
{
    struct sockaddr_in	tcp_srv_addr;   /* server's Internet socket addr */
    struct hostent	tcp_host_info;	/* from gethostbyname() */

    int			fd	= -1;
    unsigned long	inaddr;
    struct servent	*sp;
    struct hostent	*hp;	

    memset(&tcp_srv_addr, 0, sizeof(tcp_srv_addr));

    /*
     *  First try to convert the host name as a dotted-decimal number.
     *  Only if that fails do we call gethostbyname().
     */
    if ( (inaddr = inet_addr(host)) != INADDR_NONE) {
						/* it's dotted-decimal */
	tcp_host_info.h_name = NULL;
    }
    else{
	if ( (hp = gethostbyname(host)) == NULL) {
	    cTRACE(1, perror("tcp_open: host name error"));
	    return(-1);
	}
	tcp_host_info	= *hp;    /* found it by name, structure copy */
	memcpy((char*) &inaddr, hp->h_addr, sizeof(inaddr));
    }

    if (service != NULL) {
	if ( (sp = getservbyname(service, "tcp")) == NULL) {
	    cTRACE(1, perror("libcapi:tcp_open: unknown service"));
	    return(-1);
	}
	if (port > 0) {				/* caller's value */
	    tcp_srv_addr.sin_port = htons(port);
	}
	else {
						/* service's value */
	    tcp_srv_addr.sin_port = sp->s_port;
	}
    }
    else {
	if (port <= 0) {
	    cTRACE(1, perror("libcapi:tcp_open: must specify either service or port"));
		return(-1);
	}
    }

    if (port < 0) {
	cTRACE(1, perror("libcapi:tcp_open: can't get a reserved TCP port"));
	return(-1);
    }

    if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	cTRACE(1, perror("libcapi:tcp_open: can't create TCP socket"));
	return(-1);
    }

    tcp_srv_addr.sin_family		= AF_INET;
    tcp_srv_addr.sin_port		= 0;
    tcp_srv_addr.sin_addr.s_addr	= htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&tcp_srv_addr, sizeof(tcp_srv_addr)) < 0) {
	cTRACE( 1, perror("libcapi: bind"));
	goto tcp_open_error;
    }

    /*
     * Connect to the server.
     */

    tcp_srv_addr.sin_family		= AF_INET;
    tcp_srv_addr.sin_port		= htons(port);
    tcp_srv_addr.sin_addr.s_addr	= inaddr;

    if (connect(fd,(struct sockaddr*)&tcp_srv_addr, sizeof(tcp_srv_addr))<0){
	cTRACE(1, perror("libcapi:tcp_open: can't connect to server"));
	goto tcp_open_error;
    }
    return(fd);     /* all OK */

tcp_open_error:
    if (fd != -1) close(fd);
    return(-1);
}


