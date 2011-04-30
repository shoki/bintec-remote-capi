/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: Remote capi 1.1 library
 *      Author: heinz
 *    $RCSfile: rcapi.c,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: library
 *    Products: ALL
 *  Interfaces: Capi 1.1
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/
/*
 *
 * CAPI 1.1
 *
 * Remote CAPI Interface for BIANCA/BRICK Remote CAPI Client.
 *
 *
 *
 * This implementation includes the following CAPI 1.1
 * function calls:
 *
 * capi_register()
 * capi_release()
 * capi_put_message()
 * capi_get_message()
 * capi_get_manufacturer()
 * capi_get_serial()
 * capi_get_version()
 *
 *
 * The destination IP address of the Remote CAPI Server
 * (BIANCA/BRICK) is set via environment variable 'CAPI_HOST'.
 *
 * The destination TCP-Port number of the Remote CAPI Server
 * (BIANCA/BRICK) is set via environment variable 'CAPI_PORT'.
 *
 */



#include "capiconf.h"


#ifndef	TRACE_DISABLED
static int ctracelevel1 	= 1;
#define cTRACE(l, c)	if ((l) <= ctracelevel1) { c; }
#else
#define cTRACE(l, c)	/* do nothing */
#endif

int capi_errno 		= 0;


/*------------------------------------------------------------------------

	CAPI interface functions

--------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 *
 * CAPI 1.1 interface library
 *
 *------------------------------------------------------------------------*/

/************************************************************************/
/*									*/
/*	capi_open							*/
/*      returns the file descriptor of the socket (or -1 in case of an  */
/*      error). The file descriptor will be the future application ID   */
/*      in the sense of the CAPI.                                       */
/*									*/
/************************************************************************/
int capi_open()
{
    int         fd              = -1;
    char        *pHost          = NULL;
    char        *pService       = NULL;
    int         port            = CAPI_PORT_DEFAULT;

    /* Get name and port of the CAPI remote server */
    if( capi_read_environ(&pHost, &port)) {
	goto error;
    }

    /* open socket and TCP connection to the server */
    if( (fd=capi_tcp_open(pHost, pService, port)) < 0) {
	goto error;
    }
    return(fd); /* OK */

error:
    if(fd && (fd != -1) ){
	close(fd);
    }
    return(-1);
}

/************************************************************************/
/*									*/
/*	capi_close							*/
/*      closes the socket and return error code of close.               */
/*									*/
/************************************************************************/
int capi_close(fd)
int fd;
{
    int ret = 0;

    if(fd && (fd != -1) ) {
	close(fd);
    }
    return ret;
}

/************************************************************************/
/*									*/
/*	capi_set_signal (not implemented)                               */
/*									*/
/************************************************************************/
int capi_set_signal(int		appl, 
		    void	*sig)
{
    return -1;
}

/************************************************************************/
/*									*/
/*	capi_register							*/
/*                                                                      */
/*      A TCP connection to the remote CAPI server is built up. Name    */
/*      and optionally port number are taken from the variables         */
/*      CAPI_HOST and CAPI_PORT.                                        */
/*                                                                      */
/*      The file descriptor of the socket if the function value and     */
/*      serves as application ID in the sense of the CAPI. A return     */
/*      value of -1 means an error. In that case the CAPI error code    */
/*      is found in capi_errno.                                         */
/*                                                                      */
/*      CAPI user concept:                                              */
/*      Authentication on the remote CAPI server is controlled by the   */
/*      variables CAPI_USER and CAPI_PASSWD. If CAPI_USER is not set    */
/*      the authentication is skipped.                                  */
/*									*/
/************************************************************************/
int capi_register(int		msgcnt,
		  int		level3cnt,
		  int		datablkcnt,
		  int		datablklen,
		  char  *	datablock)
{
    int ev, fd = -1;
    CAPI_register_req_t req;
    struct {
	CAPI_register_conf_t conf;
	char buffer[256];
    } a;
    union CAPI_primitives *cmsg;

    memset(&req, 0, sizeof(req));
    memset(&a, 0, sizeof(a));

    if ((fd = capi_open()) == -1) {
	cTRACE( 1, perror("capi_open"));
	capi_errno = CAPI_E_REGISTER;
	goto error;
    }

    if (capi2_checkuser(fd)) {   /* check CAPI user authentication */
        cTRACE(1, fprintf(stderr, "CAPI authentication error\n"));
        capi_errno = capi2_errno;
        goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_REGISTER_REQ);
    PUT_WORD(  req.messid       , 0);

    PUT_WORD(  req.nmess	, msgcnt);
    PUT_WORD(  req.nconn	, level3cnt);
    PUT_WORD(  req.ndblock	, datablkcnt);
    PUT_WORD(  req.dblocksiz	, datablklen);

    PUT_BYTE(  req.version      , 1);	/*  CAPI 1.1 */

    if ((ev = capi_put_message( fd, (char *)&req))==CAPI_E_CONTROLLERFAILED) {
	cTRACE( 1, fprintf( stderr, "capi_put_message rn=0x%x", ev));
	capi_errno = ev;
	goto error;
    }

    while ((ev = capi_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI_E_NOMSG) break;
    }
    if (ev) { capi_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_REGISTER_CONF
	&& GET_WORD(a.conf.info) == 0
    ) {
	return fd;
    }

error:
    cTRACE( 1, fprintf( stderr, "capi_register_req failed\n"));
    capi_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi_get_manufacturer						*/
/*      In case of a negative function value (error) the CAPI error     */
/*      code is found in capi_errno.                                    */
/*									*/
/************************************************************************/
int capi_get_manufacturer(char  *buffer)
{
    int ev, fd = -1;
    CAPI_getmanufact_req_t	req;
    struct {
	CAPI_getmanufact_conf_t conf;
	char buffer[256];
    } a;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&a;

    memset(&req, 0, sizeof(req));

    if ((fd = capi_open()) == -1) {
	cTRACE( 1, perror("capi_open"));
	capi_errno = CAPI_E_REGISTER;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETMANUFACT_REQ);
    PUT_WORD(  req.messid       , 0);

    if ((ev = capi_put_message( fd, (char *)&req))==CAPI_E_CONTROLLERFAILED) {
	cTRACE( 1, fprintf( stderr, "capi_put_message rn=0x%x", ev));
	capi_errno = ev;
	goto error;
    }

    while ((ev = capi_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI_E_NOMSG) break;
    }
    if (ev) { capi_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETMANUFACT_CONF
	&& a.conf.structlen
    ) {
	memcpy( buffer, a.buffer, a.conf.structlen);

	capi_close(fd);
	return 0;
    }

error:
    capi_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi_get_version						*/
/*      In case of an error a negative function value is returned. The  */
/*      CAPI error code is passed in capi_errno.                        */
/*									*/
/************************************************************************/
int capi_get_version(char  *	buffer)
{
    int	ev = 0;
    int	fd = -1;
    CAPI_getversion_req_t  req;
    struct {
	CAPI_getversion_conf_t conf;
	char buffer[100];
    } a;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&a;

    memset(&req, 0, sizeof(req));

    if ((fd = capi_open()) == -1) {
	cTRACE( 1, perror("capi_open"));
	capi_errno = CAPI_E_REGISTER;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETVERSION_REQ);
    PUT_WORD(  req.messid       , 0);

    if ((ev = capi_put_message( fd, (char *)&req)) == CAPI_E_CONTROLLERFAILED) {
	cTRACE( 1, perror("capi_put_message"));
	capi_errno = ev;
	goto error;
    }


    while ((ev = capi_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI_E_NOMSG) break;
    }
    if (ev) { capi_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETVERSION_CONF && a.conf.structlen) {
	memcpy( buffer, a.buffer, a.conf.structlen);

	capi_close(fd);
	return 0;
    }

error:
    capi_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi_get_serial							*/
/*      A negative function value indicates an error. The CAPI error    */
/*      code is found in capi_errno in ths case.                        */
/*									*/
/************************************************************************/
int capi_get_serial(char *buffer )
{
    int ev = 0;
    int fd = -1;
    CAPI_getserial_req_t  req;
    struct {
	CAPI_getserial_conf_t conf;
	char buffer[100];
    } a;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&a;

    memset(&req, 0, sizeof(req));

    if ((fd = capi_open()) == -1) {
	cTRACE( 1, perror("capi_open"));
	capi_errno = CAPI_E_REGISTER;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(a));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETSERIAL_REQ);
    PUT_WORD(  req.messid       , 0);

    if ((ev=capi_put_message( fd, (char *)&req)) == CAPI_E_CONTROLLERFAILED) {
	cTRACE( 1, perror("capi_put_message"));
	capi_errno = ev;
	goto error;
    }

    while ((ev = capi_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI_E_NOMSG) break;
    }
    if (ev) { capi_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETSERIAL_CONF
	&& a.conf.structlen
    ) {
	memcpy( buffer, a.buffer, a.conf.structlen);

	capi_close(fd);
	return 0;
    }

error:
    capi_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi_release							*/
/*      no error code                                                   */
/*									*/
/************************************************************************/
int capi_release(int   	appl)
{
    capi_close(appl);
    return 0;
}

/************************************************************************/
/*									*/
/*	capi_put_message						*/
/*      The function value is the return code in the sense of the CAPI  */
/*									*/
/************************************************************************/
int capi_put_message(int 	appl,
		     char  *	a)
{
    union CAPI_primitives *msg;
    int		msglen;
    int		mode;
    size_t	datalen = 0;
    size_t	protlen = 0;
    char	*data = NULL;
    unsigned char arr[sizeof(unsigned short)];

    msg = (union CAPI_primitives *)a;

    protlen = GET_LEN(msg);

    if (GET_PRIMTYPE(msg) == CAPI_DATAB3_REQ) {
	datalen = GET_WORD(msg->datab3_req.datalen);
	data    = (char *)GET_DWORD(msg->datab3_req.data);
    }

    msglen = sizeof(unsigned short) + protlen + datalen;

    arr[0] = (msglen >> 8) & 0xff;
    arr[1] = msglen & 0xff;

    mode = fcntl( appl, F_GETFL);
    if (mode & O_NDELAY) fcntl( appl, F_SETFL, mode &~ O_NDELAY);

    if (capi_blockwrite( appl, arr, sizeof(arr)) != sizeof(arr)) {
	cTRACE( 1, fprintf( stderr, "capi_put_message: write msglength failed\n"));
	goto error;
    }

    if (capi_blockwrite( appl, a, protlen) != protlen) {
	cTRACE( 1, fprintf( stderr, "capi_put_message: write msgheader failed\n"));
	goto error;
    }
    if (datalen) {
	if (capi_blockwrite( appl, data, datalen) != datalen) {
	    cTRACE( 1, fprintf( stderr, "capi_put_message: write msgdata failed\n"));
	    goto error;
	}
    }
    if (mode & O_NDELAY) fcntl( appl, F_SETFL, mode);

    cTRACE( 2, fprintf( stderr, "%s\n", capi_msg(msg)));
    return 0;

error:
    if (mode & O_NDELAY) fcntl( appl, F_SETFL, mode);

    capi_errno = CAPI_E_CONTROLLERFAILED;
    return capi_errno;
}



/************************************************************************/
/*									*/
/*	capi_get_message						*/
/*      The function value is the return code in the sense of the CAPI  */
/*									*/
/************************************************************************/
int capi_get_message(int 			appl,
		     union CAPI_primitives **	cpp,
		     union CAPI_primitives *	cmsg,
		     char *			dbuf,
		     int			dlen)
{
    fd_set              readfds, writefds, errorfds;
    struct timeval      tvs;
    int			msglen = 0;
    int			mode;
    int			rSize;
    int                 RxBytes;
    char	        *Buffer;
    int			Offset;
    int			ready;
    int 		state;
    unsigned char	arr[sizeof(unsigned short)];

    FD_ZERO( &readfds);
    FD_ZERO( &writefds);
    FD_ZERO( &errorfds);

    FD_SET( appl, &readfds);
    FD_SET( appl, &errorfds);

    tvs.tv_sec = 0;
    tvs.tv_usec = 0;

    Buffer = (char *)&arr[0];
    rSize = 2;
    state = 0;
    Offset = 0;
    ready = 0;

    /* Check whether a message is available. If not return CAPI_E_NOMSG */

    if (select(appl+1, &readfds, &writefds, &errorfds, &tvs) < 0) {
	/* FIXME: What happens if errno == EINTR */
        cTRACE(1, fprintf(stderr, "select in capi2_get_message failed with code %d\n", errno));
        return (capi_errno = CAPI_E_LOCABORT);
    }

    if (FD_ISSET(appl, &errorfds)) {
        cTRACE(1, fprintf(stderr, "select detected error conditions on socket %d\n", appl));
        return (capi_errno = CAPI_E_LOCABORT);
    }

    if (!FD_ISSET(appl, &readfds)) {               /* no single byte available */
        return (capi_errno = CAPI_E_NOMSG);
    }
    
    /* Auf empfange Daten / Messages prüfen */
    if ((RxBytes  = read(appl,Buffer,2)) <= 0) {
	if (RxBytes == -1)
	    switch (errno) {
		case EINTR:
		case EAGAIN:
		   break; 

		 default:
		   fprintf(stderr,"ERROR capi2_get_message (read %d)\n",errno);
		   return (capi_errno = CAPI_E_LOCABORT);
	    }
	return (capi_errno = CAPI_E_NOMSG);
    }
    
    /* Es sind Messages / Daten vorhanden */
    rSize -= RxBytes;
    Offset += RxBytes;

    mode = fcntl( appl, F_GETFL);
    fcntl( appl, F_SETFL, mode & ~(O_NONBLOCK | O_NDELAY)); /* ??? who understands this one ??? */

    /* Warten bis komplett empfangen */
    while (!ready) {
	/* Versuche gewünschte Anzahl von Bytes zu lesen (rSize > 0) */
	if (rSize && ((RxBytes = read(appl,&Buffer[Offset],rSize)) > 0)) {
	    rSize -= RxBytes;
	    Offset += RxBytes;
	}
	
        /* Ein neuer Status */
	if (!rSize)
	    switch (state) {

		/* Gesamtlänge der Message (incl. Daten) einlesen */
		case 0:
		    msglen = (((unsigned char)(Buffer[0]) << 8) +
			       (unsigned char)(Buffer[1]))-2;

		    Buffer = (char *)cmsg;
		    Offset = 0;
		    state = 1;
		    rSize = 2;
		    break;
		
		/* Länge der CAPI Message einlesen */
		case 1:
		    rSize  = (((unsigned char)(Buffer[1]) << 8) +
			       (unsigned char)(Buffer[0]));
		    msglen -= rSize;
		    rSize -= 2;
		    state = 2;
		    break;

		/* Message komplett -> sind Daten im Anhang */    
		case 2:
                    if (GET_PRIMTYPE(cmsg) == CAPI_DATAB3_IND) {
                        PUT_DWORD( cmsg->datab3_ind.data, (unsigned long)dbuf);
			Buffer = dbuf;
			Offset = 0;
			rSize = msglen;
			state = 3;
		    }
		    else {
			/* Aktion erfolgreich beendet (keine Daten) */
			ready = 1;
                        if (msglen) printf("capi_get_message: Fatal error!\n");
                    }
		    break;
		    
		/* Daten komplett eingelesen */
		case 3:
		    /* Aktion erfolgreich beendet */
		    ready = 1;
		    break;
	    }
    }
    
    if (cpp) 
	*cpp = cmsg;

    fcntl( appl, F_SETFL, mode);
    
    return (capi_errno = 0);
}


