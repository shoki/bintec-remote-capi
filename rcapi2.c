/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: Remote Capi 2.0 library
 *      Author: heinz
 *    $RCSfile: rcapi2.c,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: library
 *    Products: ALL
 *  Interfaces: Capi 2.0
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************
 *
 *
 * CAPI 2.0
 *
 * Remote CAPI Interface for BIANCA/BRICK Remote CAPI Client.
 *
 *
 *
 * This implementation includes the following CAPI 2.0
 * function calls:
 *
 * capi2_register()
 * capi2_release()
 * capi2_put_message()
 * capi2_get_message()
 * capi2_get_manufacturer()
 * capi2_get_serial()
 * capi2_get_version()
 * capi2_get_profile()
 * capi2_wait_for_signal()
 * 
 *
 *
 * The destination IP address of the Remote CAPI Server
 * (BIANCA/BRICK) is set via environment variable 'CAPI_HOST'.
 *
 * The destination TCP-Port number of the Remote CAPI Server
 * (BIANCA/BRICK) is set via environment variable 'CAPI_PORT'.
 *
 * The user name on the Remote CAPI Server is set via the variable 
 * 'CAPI_USER' and the password via 'CAPI_PASSWD'. If CAPI_USER is 
 * not set no authentication is done.
 *
 */

#include "capiconf.h"

#ifndef	TRACE_DISABLED
static int ctracelevel2 	= 1;
#define cTRACE(l, c)	if ((l) <= ctracelevel2) { c; }
#else
#define cTRACE(l, c)	/* do nothing */
#endif


/********************************************************************
 *    GLOBAL DATA
 *******************************************************************/
int capi2_errno		= 0;

/*------------------------------------------------------------------------

	CAPI interface functions

--------------------------------------------------------------------------*/




/*------------------------------------------------------------------------

	CAPI 2.0 interface library

-------------------------------------------------------------------------*/

/************************************************************************/
/*									*/
/*	capi2_open							*/
/*      for internal use only; returns file descriptor or -1 (=error)   */
/*      fd is the file descriptor of the socket and will be the         */
/*      future application ID in the sense of the CAPI.                 */
/*									*/
/************************************************************************/
int capi2_open()
{
    rcapi_host_t capihost;

    bzero(&capihost, sizeof(capihost));

    /* get host name and CAPI port number */
    if( capi_read_environ(&(capihost.hostname), &(capihost.port))) {
	return -1;
    }
    /* open socket and TCP connection to the CAPI server */
    return(rcapi_open(&capihost));
}

/************************************************************************
 *
 *      rcapi_open
 *      for internal use only; returns file descriptor or -1 (=error)
 *      fd is the file descriptor of the socket and will be the
 *      future application ID in the sense of the CAPI.
 *
 ************************************************************************/
int rcapi_open(rcapi_host_t *host)
{ 
    int         fd              = -1;
    
    if( host->hostname == NULL ) { /* service or not XXX */
	/* missing error */
	goto error;
    }
    host->port = ( host->port) ?  host->port : CAPI_PORT_DEFAULT;

    /* open socket and TCP connection to the CAPI server */
    if( (fd=capi_tcp_open(host->hostname, 
			  host->service, 
			  host->port)) < 0) {
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
/*	capi2_close							*/
/*     closes the socket and returns error code of close()              */
/*									*/
/************************************************************************/
int capi2_close(int fd)
{
    int	ret = 0;

    if(fd && (fd != -1) ) {
	ret = close(fd);
    }

    if (ret) cTRACE(1, fprintf(stderr, "ERROR: close() on socket %d returned %d\n", fd, ret));

    return ret;
}


/************************************************************************/
/*									*/
/*	capi2_set_signal (not implemented)                              */
/*									*/
/************************************************************************/
int capi2_set_signal(int	appl, 
		     void	*sig)
{
    return -1;
}

/************************************************************************/
/*									*/
/*	capi2_wait_for_signal						*/
/*      timeout has to be specified in ms; -1 blocks until a message    */
/*      is available.                                                   */
/*      return code is function value (see CAPI specification)          */
/*      case of timeout leads to CAPI2_E_MSG_QUEUE_EMPTY                */
/*      return code 0 indicates an incoming message                     */
/*									*/
/************************************************************************/
int capi2_wait_for_signal(int	 appl, 	
			  int	 timeout)
{
    fd_set         readfds, writefds, errorfds;
    struct timeval tvs, *tvp;

    if ( appl < 0 ) {
        cTRACE( 1, fprintf( stderr, "ERROR in capi2_wait_for_signal: got illegal parameter: appl = %d.  \n", appl) );
        return CAPI2_E_ILLEGAL_APPLICATION;
    }

    while (1) {
        FD_ZERO( &readfds);
        FD_ZERO( &writefds);
        FD_ZERO( &errorfds);

        FD_SET( appl, &readfds);
        FD_SET( appl, &errorfds);

        if ( timeout == -1 ) { /* block and wait until some event occurs */
            tvp = NULL;
        } else if ( timeout == 0 ) { /* return immediately, even without event */
            tvs.tv_sec  = 0;
            tvs.tv_usec = 0;
            tvp = &tvs;
        } else if ( timeout > 0 ) { /* return on event, or after timeout */
            tvs.tv_sec  = timeout / 1000;
            tvs.tv_usec = (timeout % 1000) * 1000;
            tvp = &tvs;
        } else {
            cTRACE( 1, fprintf( stderr, "ERROR in capi2_wait_for_signal: got illegal parameter: timeout = %d.\n", timeout) );
            return CAPI2_E_ILLEGAL_MSG_PARAM_CODING;
        }
  
        cTRACE( 3, fprintf( stderr, "capi2_wait_for_signal: starting select() on appl = %d...\n", appl) );

        switch ( select( appl+1, &readfds, &writefds, &errorfds, tvp) ) {

            case -1: /* OS error */
                if (errno == EINTR) {
	            cTRACE( 2, fprintf( stderr, "capi2_wait_for_signal: select() was interrupted by a signal.\n") );
        	    continue;
                } else if (errno == EBADF) {
                    cTRACE( 1, fprintf( stderr, "ERROR in capi2_wait_for_signal: select() returned EBADF, appl ID is = %d.\n", appl) );
                    return CAPI2_E_ILLEGAL_APPLICATION;
                } else { /* errno = EINVAL or something else not specified */
                    cTRACE( 1, perror( "ERROR in capi2_wait_for_signal: select() ret urned") );
                    return CAPI2_E_MSG_QUEUE_OVERFLOW; /* force application to re-register */
                }

            case 0:  /* time out */
                cTRACE( 2, fprintf( stderr, "capi2_wait_for_signal: select() timed out.\n") );
                return CAPI2_E_MSG_QUEUE_EMPTY;

            default: /* some fds got an event */
                if ( FD_ISSET( appl, &readfds) ) {
           	/* incoming CAPI message */
                    cTRACE( 3, fprintf( stderr, "capi2_wait_for_signal: exiting successful for appl = %d.\n", appl) );
	            return 0;
                } else if ( FD_ISSET ( appl, &errorfds) ) {
                /* got an error, but don't know which; force application to re-register */
	            cTRACE( 1, fprintf( stderr, "ERROR in capi2_wait_for_signal: select() returned an error event for appl = %d.\n", appl) );
                    return CAPI2_E_MSG_QUEUE_OVERFLOW;
                } else {
                /* got something, but don't know what; force application to re-register */
                    cTRACE( 1, fprintf( stderr, "ERROR in capi2_wait_for_signal: select() returned something unhandle d.\n") );
                    return CAPI2_E_MSG_QUEUE_OVERFLOW;
                }
        }
    }
}

/************************************************************************/
/*									*/
/*	capi2_register							*/
/*      function value is the application ID (file descriptor of the    */
/*      socket).                                                        */
/*      a negative value means that the CAPI return code is found in    */
/*      capi2_errno                                                     */
/*                                                                      */
/*      CAPI user concept:                                              */
/*      authentication is controlled by the two environment variables   */
/*      CAPI_USER , CAPI_PASSWD                                         */
/*      If CAPI_USER is not set no authentication is done.              */
/*                                                                      */
/*      Remote CAPI concept:                                            */
/*      A TCP connection is built up to the Remote CAPI Server to pass  */
/*      all CAPI messages over this connection. An open failure for     */
/*      the connection leads to error code                              */
/*      CAPI2_E_REG_CAPI_NOT_INSTALLED.                                 */
/*      The remote CAPI server is specified in the varaibles CAPI_HOST  */
/*      and CAPI_PORT (optional).                                       */
/*									*/
/************************************************************************/
int capi2_register(int		msgsize,
		   int		level3cnt,
		   int		datablkcnt,
		   int		datablklen,
		   char		*datablock)
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

    if ((fd = capi2_open()) == -1) {
	cTRACE( 1, perror("capi2_open"));
	capi2_errno = CAPI2_E_REG_CAPI_NOT_INSTALLED;
	goto error;
    }

    if (capi2_checkuser(fd)) { /* check CAPI user authentication */
        cTRACE(1, fprintf(stderr, "CAPI authentication error\n"));
        goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_REGISTER_REQ);
    PUT_WORD(  req.messid       , 0);

    PUT_WORD(  req.nmess	, msgsize);
    PUT_WORD(  req.nconn	, level3cnt);
    PUT_WORD(  req.ndblock	, datablkcnt);
    PUT_WORD(  req.dblocksiz	, datablklen);

    PUT_BYTE(  req.version      , 2);	/*  CAPI 2.0  */

    if ((ev = capi2_put_message( fd, (char *)&req)) != 0) {
	cTRACE( 1, fprintf( stderr, "capi2_put_message rn=0x%x", ev));
	capi2_errno = ev;
	goto error;
    }

    while ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI2_E_MSG_QUEUE_EMPTY) break;
    }
    if (ev) { capi2_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_REGISTER_CONF
	&& GET_WORD(a.conf.info) == 0
    ) {
	return fd; /* use file descriptor of socket as application ID */
    }

error:
    cTRACE( 1, fprintf( stderr, "capi2_register_req failed\n"));
    capi2_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi2_get_manufacturer						*/
/*      a negative funtion value means that the CAPI error code is      */
/*      found in capi2_errno                                            */
/*									*/
/************************************************************************/

int capi2_get_manufacturer(char *buffer)
{
    rcapi_host_t capihost;

    bzero(&capihost, sizeof(capihost));

    capi_read_environ(&(capihost.hostname), &(capihost.port));
    return(rcapi_get_manufacturer(buffer, &capihost));

}

/***********************************************************************
 *
 *	rcapi_get_manufacturer
 *	- copy of capi2_get_manufacturer
 *	
 *      a negative funtion value means that the CAPI error code is
 *      found in capi2_errno
 *
 ***********************************************************************/


int rcapi_get_manufacturer(char		*buffer, 
				rcapi_host_t	*host)
{
    int ev, fd = -1;
    CAPI_getmanufact_req_t	req;
    struct {
	CAPI_getmanufact_conf_t conf;
	char buffer[256];
    } a;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&a;

    memset(&req, 0, sizeof(req));

    if ((fd = rcapi_open(host)) == -1) {
	cTRACE( 1, perror("rcapi_open"));
	capi2_errno = CAPI2_E_CAPI_NOT_INSTALLED;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETMANUFACT_REQ);
    PUT_WORD(  req.messid       , 0);

    if ((ev = capi2_put_message( fd, (char *)&req)) != 0) {
	cTRACE( 1, fprintf( stderr, "capi2_put_message rn=0x%x", ev));
	capi2_errno = ev;
	goto error;
    }

    while ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI2_E_MSG_QUEUE_EMPTY) break;
    }
    if (ev) { capi2_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETMANUFACT_CONF
	&& a.conf.structlen	) {
	memcpy( buffer, a.buffer, MIN((size_t)a.conf.structlen, sizeof(a.buffer)));
	capi2_close(fd);
	return 0;
    }

error:
    capi2_close(fd);
    return -1;
}



/************************************************************************/
/*									*/
/*	capi2_get_version						*/
/*      negative function value means that CAPI error code is in        */
/*      capi2_errno                                                     */
/*									*/
/************************************************************************/
int capi2_get_version(unsigned long *lPtrVersion )
{
    rcapi_host_t	capihost;
    bzero(&capihost, sizeof(capihost));

    capi_read_environ(&(capihost.hostname), &(capihost.port));
    return(rcapi_get_version(lPtrVersion, &capihost));
}



int rcapi_get_version(unsigned long 	*lPtrVersion,
			   rcapi_host_t	*host)
{
    int ev = 0;
    int fd = -1;
    CAPI_getversion_req_t  req;
    struct {
	CAPI_getversion_conf_t conf;
	char buffer[100];
    } a;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&a;

    memset(&req, 0, sizeof(req));

    if ((fd = rcapi_open(host)) == -1) {
	cTRACE( 1, perror("capi2_open"));
	capi2_errno = CAPI2_E_CAPI_NOT_INSTALLED;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETVERSION_REQ);
    PUT_WORD(  req.messid       , 0);

    if ((ev=capi2_put_message( fd, (char *)&req)) != 0) {
	cTRACE( 1, perror("capi2_put_message"));
	capi2_errno = ev;
	goto error;
    }


    while ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI2_E_MSG_QUEUE_EMPTY) break;
    }
    if (ev) { capi2_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETVERSION_CONF && a.conf.structlen) {
	*lPtrVersion = GET_DWORD(a.conf.version);
	capi2_close(fd);
	return 0;
    }

error:
    capi2_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi2_get_serial						*/
/*      in case that the function value is negative the CAPI error code */
/*      is found in capi2_errno                                         */
/*									*/
/************************************************************************/
int capi2_get_serial(char *buffer )
{
    rcapi_host_t	capihost;
    bzero(&capihost, sizeof(capihost));
    capi_read_environ(&(capihost.hostname), &(capihost.port));
    return(rcapi_get_serial(buffer, &capihost));
}


int rcapi_get_serial(char			*buffer,
			  rcapi_host_t	*host)
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

    if ((fd = rcapi_open(host)) == -1) {
	cTRACE( 1, perror("rcapi_open"));
	capi2_errno = CAPI2_E_CAPI_NOT_INSTALLED;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(a));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETSERIAL_REQ);
    PUT_WORD(  req.messid       , 0);

    if ((ev=capi2_put_message( fd, (char *)&req)) != 0) {
	cTRACE( 1, perror("capi2_put_message"));
	capi2_errno = ev;
	goto error;
    }

    while ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	if (ev != CAPI2_E_MSG_QUEUE_EMPTY) break;
    }
    if (ev) { capi2_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETSERIAL_CONF && a.conf.structlen) {
	memcpy( buffer, a.buffer, a.conf.structlen);

	capi2_close(fd);
	return 0;
    }

error:
    capi2_close(fd);
    return -1;
}

/************************************************************************/
/*									*/
/*	capi2_get_profile						*/
/*      a negative function value indicates an error. The CAPI error    */
/*      code is found in capi2_errno in this case.                      */
/*									*/
/************************************************************************/
int capi2_get_profile(int			nCtrl, 
		      struct capi_getprofile	*prof )
{
    rcapi_host_t	capihost;
    bzero(&capihost, sizeof(capihost));

    capi_read_environ(&(capihost.hostname), &(capihost.port));
    return(rcapi_get_profile(nCtrl, prof, &capihost));
}


int rcapi_get_profile(int			nCtrl, 
		      struct capi_getprofile	*prof,
		      rcapi_host_t		*host)
{
    int ev = 0;
    int fd = -1;
    CAPI_getprofile_req_t  req;
    CAPI_getprofile_conf_t conf;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&conf;

    memset(&req, 0, sizeof(req));

    if ((fd = rcapi_open(host)) == -1) {
	cTRACE( 1, perror("rcapi_open"));
	capi2_errno = CAPI2_E_CAPI_NOT_INSTALLED;
	goto error;
    }

    PUT_WORD(  req.len          , sizeof(req));
    PUT_WORD(  req.appl         , fd);
    PUT_WORD(  req.PRIM_type    , CAPI_GETPROFILE_REQ);
    PUT_WORD(  req.messid       , 0);

    PUT_WORD(  req.ncontrl	, nCtrl);

    if ((ev=capi2_put_message( fd, (char *)&req)) != 0) {
	cTRACE( 1, perror("capi2_put_message"));
	capi2_errno = ev;
	goto error;
    }

    while ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&conf, NULL, 0)) != 0) {
	if (ev != CAPI2_E_MSG_QUEUE_EMPTY) break;
    }
    if (ev) { capi2_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_GETPROFILE_CONF &&
	    GET_WORD(conf.info) == 0) {
	prof->ncontrl    = GET_WORD( conf.ncontrl);
	prof->nchannel   = GET_WORD( conf.nchannel);
	prof->options    = GET_WORD( conf.options);
	prof->b1protocol = GET_WORD( conf.b1protocol);
	prof->b2protocol = GET_WORD( conf.b2protocol);
	prof->b3protocol = GET_WORD( conf.b3protocol);
	memcpy( prof->reserved,    conf.reserved,    sizeof(conf.reserved));
	memcpy( prof->manufacturer,conf.manufacturer,sizeof(conf.manufacturer));

	capi2_close(fd);
	return 0;
    }
    cTRACE( 1, fprintf( stderr, "capi2_get_profile: 0x%x\n",
						    GET_WORD(conf.info)));

error:
    capi2_close(fd);
    return -1;
}


/************************************************************************/
/*									*/
/*	capi2_release							*/
/*      no return code provided.                                        */
/*									*/
/************************************************************************/
int capi2_release(int fd )
{
    capi2_close(fd);
    return 0;
}

/************************************************************************/
/*									*/
/*	capi2_put_message						*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
int capi2_put_message(int appl, 
		      char *a)
{
    union CAPI_primitives *msg;
    int msglen, ret;
    int datalen = 0, protlen = 0;
    char *data = NULL;
    unsigned char arr[sizeof(unsigned short)];
    int error = 0;

    struct iovec iov[3];

    msg = (union CAPI_primitives *)a;

    protlen = GET_LEN(msg);

    if (GET_PRIMTYPE(msg) == CAPI2_DATAB3_REQ) {
	datalen = GET_WORD(msg->c2datab3_req.datalen);
	data    = (char *)GET_DWORD(msg->c2datab3_req.dataptr);
    }

    msglen = sizeof(unsigned short) + protlen + datalen;

    arr[0] = (msglen >> 8) & 0xff;
    arr[1] = msglen & 0xff;

    iov[0].iov_base = (caddr_t)arr;
    iov[0].iov_len  = sizeof(arr);

    iov[1].iov_base = (caddr_t)a;
    iov[1].iov_len  = protlen;

    iov[2].iov_base = (caddr_t)data;
    iov[2].iov_len  = datalen;

    /* see man page of writev */
    while ((ret = writev( appl, iov, 3)) < 0 && errno == EINTR);

    if ( ret == msglen ) {  /* success */
	/* cTRACE( 2, fprintf( stderr, "%s\n", capi_msg((union CAPI_primitives *)a))); */

    } else if ( ret > 0 ) { /* message sent uncompletly */
        fprintf( stderr, "capi2_put_message: FATAL ERROR!\n");
        fprintf( stderr, "    writev() has written only %d of %d bytes!\n", ret, msglen);
	fprintf( stderr, "    TCP/IP stream syncronisation may have been lost!\n");
	fprintf( stderr, "    Can't recover due to multi thread environment!\n");
	error = CAPI2_E_MSG_QUEUE_OVERFLOW; /* force application to reregister */

    } else if ( ret == -1 ) { /* OS error occured */
        switch ( errno ) {
        case EAGAIN:
	case EDEADLK:
	  error = CAPI2_E_INTERNAL_BUSY_CONDITION;
	  break;
        case EBADF:
	  error = CAPI2_E_ILLEGAL_APPLICATION;
	  break;
	case EFAULT:
	case ERANGE:
	case EINVAL:
	  error = CAPI2_E_ILLEGAL_MSG_PARAM_CODING;
	  break;
#ifdef ENOSR
	case ENOSR:
	  error = CAPI2_E_OS_RESOURCE_ERROR;
	  break;
#endif
	case ENXIO:
	  error = CAPI2_E_MSG_QUEUE_OVERFLOW;
	  break;
	case EDQUOT:
	case EFBIG:
	case EIO:
	case ENOLCK:
#ifdef ENOLINK
	case ENOLINK:
#endif
	case ENOSPC:
	case EPIPE:
	case ESPIPE:
        default: /* don't know what happend, force application to reregister */
	  perror("capi2_put_message: ERROR! writev() returned unhandled errno");
	  error = CAPI2_E_MSG_QUEUE_OVERFLOW;
      }

    } else { /* don't know what happend, force application to reregister */
        fprintf( stderr, "capi2_put_message: ERROR! writev() returned unhandled value: %d\n", ret);
	error = CAPI2_E_MSG_QUEUE_OVERFLOW;
    }

    return (capi2_errno = error);
}

/************************************************************************/
/*									*/
/*	capi2_get_message						*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*      CAPI2_E_MSG_QUEUE_EMPTY is returned if no message is available  */
/*      (in other words capi2_get_message does not block).              */
/*      Calls of capi2_get_message must be serialized by the application*/
/*									*/
/************************************************************************/
int capi2_get_message(int 			appl,
		      union CAPI_primitives **	cpp,
		      union CAPI_primitives *  	cmsg,
		      char *		       	dbuf,
		      int		       	dlen)
{
    fd_set              readfds, writefds, errorfds;
    struct timeval      tvs;
    int			msglen = 0;
    int			rSize;
    int                 RxBytes;
    char	        *Buffer;
    int			Offset;
    int			ready;
    int 		state;
    unsigned char	arr[sizeof(unsigned short)];
    int			ret;

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

    if ((ret = select(appl+1, &readfds, &writefds, &errorfds, &tvs) <= 0)) {
	if (ret == -1 ) {
	    switch (errno) {
		case EINTR:
		    cTRACE( 2, fprintf( stderr, "capi2_get_message: select() was interrupted by a signal.\n") );
		    return (capi2_errno = CAPI2_E_MSG_QUEUE_EMPTY);
		case EBADF:
		    cTRACE( 1, fprintf( stderr, "ERROR in capi2_get_message: select() returned EBADF, appl ID is = %d.\n", appl) );
		    return (capi2_errno = CAPI2_E_ILLEGAL_APPLICATION);
		default:
		    cTRACE( 1, perror( "ERROR in capi2_get_message: select() ret urned") );
		    return (capi2_errno = CAPI2_E_OS_RESOURCE_ERROR);
	    }
	}
	cTRACE( 3, perror( "ERROR in capi2_get_message: select() retuerned 0") );
	return (capi2_errno = CAPI2_E_MSG_QUEUE_EMPTY);
    } 

    if (FD_ISSET(appl, &errorfds)) {
        cTRACE(1, fprintf(stderr, "select detected error conditions on socket %d\n", appl));
        return (capi2_errno = CAPI2_E_OS_RESOURCE_ERROR);
    }
 
    if (!FD_ISSET(appl, &readfds)) {               /* no single byte available */
        return (capi2_errno = CAPI2_E_MSG_QUEUE_EMPTY);
    }
    
    /* Auf empfange Daten / Messages prüfen */
    if ((RxBytes  = read(appl,Buffer,2)) <= 0) {
	if (RxBytes == -1)
	    switch (errno) {
		case EINTR:   /* signal pending */
		case EAGAIN:
		   break; 

		 default:
		   fprintf(stderr,"ERROR capi2_get_message (appl %d): %s\n",
			   appl, strerror(errno));
                   return (capi2_errno = CAPI2_E_INTERNAL_BUSY_CONDITION);
	    }
        return (capi2_errno = CAPI2_E_MSG_QUEUE_EMPTY);
    }
    
    /* Es sind Messages / Daten vorhanden */
    rSize -= RxBytes;
    Offset += RxBytes;

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
		    if ((GET_WORD(cmsg->c2header.PRIM_type)) == CAPI2_DATAB3_IND) {
			PUT_DWORD( cmsg->c2datab3_ind.dataptr,
				   (unsigned long)dbuf);
			Buffer = dbuf;
			Offset = 0;
			rSize = msglen;
			state = 3;
		    }
		    else {
			/* Aktion erfolgreich beendet (keine Daten) */
			ready = 1;
		    }
		    break;
		    
		/* Daten komplett eingelesen */
		case 3:
		    /* Aktion erfolgreich beendet */
		    ready = 1;
		    break;
	    }
    }
    
    if (cpp)  {
	*cpp = cmsg;
    }

    return (capi2_errno = 0);
}
