/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *                Title: <one line description>
 *               Author: <username>
 *       $LastChangedBy: smeagle $ 
 *     $LastChangedDate: 2005-11-21 20:49:04 +0100 (Mon, 21 Nov 2005) $ 
 * $LastChangedRevision: 57 $
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
 ************************************************************************
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
 * capi2_register2()
 * capi2_checkuser()
 *
 ***********************************************************************/

#ifdef CAPI_MD5_HASH_LEN
#undef CAPI_MD5_HASH_LEN
#endif

#define CAPI_MD5_HASH_LEN 16
#define MAXRTT 6000               /* 6 sec maximal response time */

#include "capiconf.h"
#include <sys/types.h>
#include <md5.h>

#ifndef	TRACE_DISABLED
static int ctracelevel2 	= 1;
#define cTRACE(l, c)	if ((l) <= ctracelevel2) { c; }
#else
#define cTRACE(l, c)	/* do nothing */
#endif

extern int capi2_errno;


/*------------------------------------------------------------------------

	CAPI interface functions
	CAPI 2.0 interface library

-------------------------------------------------------------------------*/
/************************************************************************
 *									*
 *	capi2_register2							*
 *									*
 ***********************************************************************/
int rcapi_register(int		msgsize, 
			int		level3cnt, 
			int		datablkcnt, 
			int		datablklen, 
			char		*datablock,
			rcapi_host_t	*host,
			rcapi_auth_t	*auth)
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

    if ((fd = rcapi_open(host)) == -1) {
	cTRACE( 1, perror("capi2_open"));
	capi2_errno = CAPI2_E_REG_CAPI_NOT_INSTALLED;
	goto error;
    }
    if(auth != NULL) {
	if (capi2_checkuser2(fd, auth->user ,auth->passwd) < 0) 				return -1;
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

    while ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, 
				    NULL, 0)) != 0) {
	if (ev != CAPI2_E_MSG_QUEUE_EMPTY) break;
    }
    if (ev) { capi2_errno = ev; goto error; }

    if (GET_PRIMTYPE(cmsg) == CAPI_REGISTER_CONF
	&& GET_WORD(a.conf.info) == 0
    ) {
	return fd;
    }

error:
    cTRACE( 1, fprintf( stderr, "capi2_register_req failed\n"));
    capi2_close(fd);
    return -1;
}


/************************************************************************/
/*									*/
/*	capi2_checkuser							*/
/*      return value 0 ok. else error in capi2_errno                    */
/*      CAPI authentification is based on the variables CAPI_USER and   */
/*      CAPI_PASSWD.                                                    */
/*									*/
/************************************************************************/

int capi2_checkuser(int fd)
{
    char	*user	= NULL;
    char	*passwd	= NULL;

    if (!(user = getenv("CAPI_USER"))) {
	return 0;	/* OK no authentication desired */
    }

    if (!(passwd = getenv("CAPI_PASSWD"))) {
	passwd = (char *)"";
    }
    return (capi2_checkuser2(fd, user, passwd));
}


/************************************************************************
 *								
 *	capi2_checkuser2					
 *      CAPI authentification is based on the parameters	
 *	- user							
 *	- passwd						
 *	-> RET value 0 ok. else error in capi2_errno            
 *								
 ***********************************************************************/



int capi2_checkuser2(int	fd,
		     char	*user,
		     char	*passwd)
{
#define CCHECK_ERR -1

    int ev;
    struct {
	CAPI_control_req_t	req;
	char			buffer[256];
    } r;
    struct {
	CAPI_control_conf_ex_t	conf;
	char			buffer[256];
    } a;
    union CAPI_primitives *cmsg = (union CAPI_primitives *)&a;
    int 	len;
    int		chall_len	= 0;
    char	*chall		= NULL;
    struct userdata	*data;
    md5context_t	md5Context;
   
    memset(&r.req, 0, sizeof(r.req));

    if (user == NULL) {
	return 0;	/* OK no authentication desired */
    }
    if (passwd == NULL) {
	passwd = "";
    }


    PUT_WORD(  r.req.len          , sizeof(r.req));
    PUT_WORD(  r.req.appl         , fd);
    PUT_WORD(  r.req.PRIM_type    , CAPI_CONTROL_REQ);
    PUT_WORD(  r.req.messid       , 0);
    PUT_WORD(  r.req.contrl       , 0);
    PUT_WORD(  r.req.type         , CTRL_GETCHALLENGE);
    PUT_BYTE(  r.req.structlen    , 0);

    if ((ev = capi2_put_message( fd, (char *)&r)) != 0) {
	cTRACE( 1, fprintf( stderr, "capi2_put_message rn=0x%x\n", ev));
	capi2_errno = ev;
	return CCHECK_ERR;
    }

    if ((ev = capi2_wait_for_signal(fd, MAXRTT)) != 0) {
        cTRACE(1, fprintf(stderr, "capi2_wait_for_signal 0x%x\n", ev));
        capi2_errno = ev;
	return CCHECK_ERR;    
    }

    if ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	cTRACE( 1, fprintf( stderr, "capi2_get_message rn=0x%x\n", ev));
        capi2_errno = ev;
	return CCHECK_ERR;
    }

    if (GET_PRIMTYPE(cmsg) == CAPI_CONTROL_CONF
	&& GET_WORD(a.conf.type) == CTRL_GETCHALLENGE    /* get random number */
	&& a.conf.structlen	
    ) {
	chall_len = (size_t)a.conf.structlen;
	chall = a.buffer;  /* random number computed by the brick */
    } else {
        cTRACE( 1, fprintf( stderr, "CAPI_CONTROL_CONF failed\n"));
        capi2_errno = CAPI2_E_REG_OS_RESOURCE_ERROR;
	return CCHECK_ERR;
    }


    /*
     * calculate hash value;
     * see RFC 1321 for a description of the MD5 algorithm
     */

    MD5Init(&md5Context);
    MD5Update (&md5Context, (unsigned char *) user, strlen(user));
    MD5Update (&md5Context, chall, chall_len);
    MD5Update (&md5Context, (unsigned char *) passwd, strlen(passwd));
    MD5Final (&md5Context);

    PUT_WORD(  r.req.appl         , fd);
    PUT_WORD(  r.req.PRIM_type    , CAPI_CONTROL_REQ);
    PUT_WORD(  r.req.messid       , 0);
    PUT_WORD(  r.req.contrl       , 0);
    PUT_WORD(  r.req.type         , CTRL_SETUSER);
    /* copy user and hash value */
    data = (struct userdata *)&r.req.structlen;
 
    data->length = strlen(user);
    memcpy( data->data, user, data->length);
    data = (struct userdata *)&data->data[data->length];
 
    data->length = CAPI_MD5_HASH_LEN;


    memcpy( data->data, md5Context.digest, CAPI_MD5_HASH_LEN);  /* hash value to be compared */
    data = (struct userdata *)&data->data[data->length];        /* on the other side */
 
    len = (char *)data - (char *)&r;
    PUT_WORD(  r.req.len, len );
 
    if ((ev = capi2_put_message( fd, (char *)&r)) != 0) {
	cTRACE( 1, fprintf( stderr, "capi2_put_message rn=0x%x\n", ev));
	capi2_errno = ev;
	return CCHECK_ERR;
    }

    if ((ev = capi2_wait_for_signal(fd, MAXRTT)) != 0) {
        cTRACE(1, fprintf(stderr, "capi2_wait_for_signal 0x%x\n", ev));
        capi2_errno = ev;
	return CCHECK_ERR;
    }

    if ((ev = capi2_get_message( fd, &cmsg, (union CAPI_primitives *)&a, NULL, 0)) != 0) {
	cTRACE( 1, fprintf( stderr, "capi2_get_message rn=0x%x\n", ev));
        capi2_errno = ev;
	return CCHECK_ERR;
    }

    if (GET_PRIMTYPE(cmsg) == CAPI_CONTROL_CONF
	&& GET_WORD(a.conf.type) == CTRL_SETUSER
	&&  GET_WORD(a.conf.info) == 1
    ) {
	return 0; 		/* OK authentication passed */
    }
    cTRACE( 1, fprintf( stderr, "CAPI_CONTROL_CONF failed, invalid authentication"));
    capi2_errno = CAPI2_E_REG_EXT_EQUIPMENT_NOT_SUPPORTED;
    return CCHECK_ERR;
}
