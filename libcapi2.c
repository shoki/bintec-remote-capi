/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: capi 2.0 library
 *      Author: heinz
 *    $RCSfile: libcapi2.c,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: library
 *    Products: ALL 
 *  Interfaces: CAPI2
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/
#include "capiconf.h"

extern int capi2_errno;

/************************************************************************/
/*                                                                      */
/*      get_messid                                                      */
/*                                                                      */
/************************************************************************/

/*
 * speedup such simple functions
 */
static unsigned short messid_2 = 1;
#define get_messid() (messid_2 = (++messid_2 & 0x7fff))


/************************************************************************/
/*									*/
/*	capi2_listen_req						*/
/*      return code = function value (see CAPI specification)           */
/*									*/
/************************************************************************/
unsigned short capi2_listen_req(int 		appl,
				unsigned long 	contrl,
				unsigned long 	info_mask,
				unsigned long	cip_mask,
				unsigned long	cip_mask2,
				struct userdata *cpn,
				struct userdata *cps)
{
    struct {
	CAPI2_listen_req_t a;
	char buffer[256];
    } a;
    int len;
    struct userdata *data;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    PUT_WORD(  a.a.appl         , appl);
    PUT_WORD(  a.a.PRIM_type    , CAPI2_LISTEN_REQ);
    PUT_WORD(  a.a.messid       , usMessId);

    PUT_DWORD( a.a.contrl       , contrl);
    PUT_DWORD( a.a.info_mask    , info_mask);
    PUT_DWORD( a.a.cip_mask     , cip_mask);
    PUT_DWORD( a.a.cip_mask2    , cip_mask2);

    data = (struct userdata *)&a.a.structlen;

    if (cpn) 	memcpy( data, (char *)cpn, (size_t)cpn->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (cps) 	memcpy( data, (char *)cps, (size_t)cps->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    len = (char *)data - (char *)&a;
    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_alert_req							*/
/*      return code = function value (see CAPI specification)           */
/*									*/
/************************************************************************/
unsigned short capi2_alert_req(int		appl,
			       unsigned long	plci,
			       struct userdata	*add)
{
    struct {
	CAPI2_alert_req_t a;
	char buffer[256];
    } a;
    int len;
    struct userdata *data;
    unsigned short usMessId = get_messid();

    memset( &a.a, 0, sizeof(a.a));

    PUT_WORD(  a.a.appl,	appl);
    PUT_WORD(  a.a.PRIM_type, 	CAPI2_ALERT_REQ);
    PUT_WORD(  a.a.messid,	usMessId);
    PUT_WORD(  a.a.plci,	plci);

    data = (struct userdata *)&a.a.structlen;

    if (add) 	memcpy( data, (char *)add, (size_t)add->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    len = (char *)data - (char *)&a;
    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_connect_req						*/
/*      return value = message id or (unsigned short)-1                 */
/*      CAPI error code is in capi2_errno (see CAPI specification)      */
/*      The message id has to be checked against that in the            */
/*      confirmation.                                                   */
/*									*/
/************************************************************************/
unsigned short capi2_connect_req(int 			appl,
				 unsigned long		 contrl,
				 unsigned short		cip_value,
				 struct userdata 	*dad, 
				 struct userdata 	*oad, 
				 struct userdata 	*dsa, 
				 struct userdata 	*osa,
				 struct userdata 	*bprot,
				 struct userdata 	*bc, 
				 struct userdata 	*llc, 
				 struct userdata 	*hlc,
				 struct userdata 	*add)
{
    struct {
	CAPI2_connect_req_t a;
	char buffer[256];
    } a;
    int len;
    struct userdata *data;
    unsigned short  usMessId = get_messid();

    memset( &a.a, 0, sizeof(a.a));

    PUT_WORD(  a.a.appl,	appl);
    PUT_WORD(  a.a.PRIM_type, 	CAPI2_CONNECT_REQ);
    PUT_WORD(  a.a.messid,	usMessId);

    PUT_DWORD( a.a.contrl,	contrl);
    PUT_WORD(  a.a.cip_value,	cip_value);

    data = (struct userdata *)&a.a.structlen;

    if (dad) 	memcpy( data, (char *)dad, (size_t)dad->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (oad) 	memcpy( data, (char *)oad, (size_t)oad->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (dsa) 	memcpy( data, (char *)dsa, (size_t)dsa->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (osa) 	memcpy( data, (char *)osa, (size_t)osa->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (bprot)	memcpy( data, (char *)bprot, (size_t)bprot->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (bc) 	memcpy( data, (char *)bc, (size_t)bc->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (llc) 	memcpy( data, (char *)llc, (size_t)llc->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (hlc) 	memcpy( data, (char *)hlc, (size_t)hlc->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (add) 	memcpy( data, (char *)add, (size_t)add->length+1);
    else 	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    len = (char *)data - (char *)&a;
    PUT_WORD( a.a.len, len);

    if (capi2_put_message( appl, (char FAR *)&a) != 0) {
	return (unsigned short)-1;
    }
    return usMessId;
}


/************************************************************************/
/*									*/
/*	capi2_connect_resp						*/
/*      return code is function value (see CAPI specification)          */
/*									*/
/************************************************************************/
unsigned short capi2_connect_resp(int			appl,
				  unsigned short	messid,
				  unsigned long		plci,
				  int			reject,
				  struct userdata 	*bprot, 
				  struct userdata	*cad, 
				  struct userdata	*csa, 
				  struct userdata	*llc, 
				  struct userdata	*add)
{
    struct {
	CAPI2_connect_resp_t	a;
	char buffer[256];
    } a;
    struct userdata *data;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    PUT_WORD(    a.a.len,	sizeof(a));
    PUT_WORD(    a.a.PRIM_type,	CAPI2_CONNECT_RESP);
    PUT_WORD(    a.a.appl,  	appl);
    PUT_WORD(    a.a.messid,  	usMessId);

    PUT_DWORD(	 a.a.plci,	plci);
    PUT_WORD(	 a.a.reject, 	reject);

    data = (struct userdata *)&a.a.structlen;

    if (bprot)	memcpy( data, (char *)bprot, (size_t)bprot->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];
    
    if (cad)	memcpy( data, (char *)cad, (size_t)cad->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (csa)	memcpy( data, (char *)csa, (size_t)csa->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (llc)	memcpy( data, (char *)llc, (size_t)llc->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (add)	memcpy( data, (char *)add, (size_t)add->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    len = (char *)data - (char *)&a;
    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_connectactive_resp					*/
/*      return code is function value (see CAPI specification)          */
/*									*/
/************************************************************************/
unsigned short capi2_connectactive_resp(int 		appl,
					unsigned short 	messid,
					unsigned long 	plci)
{
    CAPI2_connectactive_resp_t a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI2_CONNECTACTIVE_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_DWORD(   a.plci      ,  plci);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_info_req							*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_info_req(int 		appl,
			      unsigned long 	ident,
			      struct userdata *cpn,
			      struct userdata *add)
{
    struct {
	CAPI2_info_req_t a;
	char buffer[256];
    } a;
    struct userdata *data;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    PUT_WORD(    a.a.len       ,  sizeof(a));
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_INFO_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.ident     ,  ident);
    data = (struct userdata *)&a.a.structlen;

    if (cpn)	memcpy( data, (char *)cpn, (size_t)cpn->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    if (add)	memcpy( data, (char *)add, (size_t)add->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    len = (char *)data - (char *)&a;
    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}


/************************************************************************/
/*									*/
/*	capi2_info_resp							*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_info_resp(int 		appl,
			       unsigned short 	messid,
			       unsigned long 	ident)
{
    CAPI2_info_resp_t a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI2_INFO_RESP);
    PUT_WORD(    a.messid    ,  messid);

    PUT_DWORD(   a.ident     ,  ident);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_disconnect_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_disconnect_req(int 		appl,
				    unsigned long 	plci,
				    struct userdata 	*add)
{
    struct {
	CAPI2_disconnect_req_t a;
	char buffer[256];
    } a;
    struct userdata *data;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    PUT_WORD(    a.a.len       ,  sizeof(a));
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_DISCONNECT_REQ);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.plci      ,  plci);

    data = (struct userdata *)&a.a.structlen;

    if (add)	memcpy( data, (char *)add, (size_t)add->length+1);
    else	data->length = 0;
    data = (struct userdata *)&data->data[data->length];

    len = (char *)data - (char *)&a;
    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_disconnect_resp						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_disconnect_resp(int 		appl,
				     unsigned short 	messid,
				     unsigned long 	plci)
{
    CAPI2_disconnect_resp_t a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.PRIM_type ,  CAPI2_DISCONNECT_RESP);
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.messid    ,  messid);
    PUT_DWORD(   a.plci      ,  plci);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_selectb_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_selectb_req(int 			appl,
				 unsigned long 		plci,
				 struct userdata *	bproto)
{
    struct {
	CAPI2_selectb_req_t    a;
	char buffer[256];
    } a;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);

    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_SELECTB_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.plci      ,  plci);

    if (bproto) {
	len += bproto->length;
	memcpy( &a.a.structlen, bproto, (size_t)bproto->length+1);
    }

    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_facility_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_facility_req(int 			appl,
				  unsigned long 	ident,
				  unsigned short	selector,
				  struct userdata *	facdata)
{
    struct {
	CAPI2_facility_req_t    a;
	char buffer[256];
    } a;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);

    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_FACILITY_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.ident     ,  ident);
    PUT_WORD(    a.a.selector  ,  selector);

    if (facdata) {
	len += facdata->length;
	memcpy( &a.a.structlen, facdata, (size_t)facdata->length+1);
    }

    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_facility_resp						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_facility_resp(int 			appl,
				   unsigned short	messid,
				   unsigned long 	ident,
				   unsigned short	selector,
				   struct userdata *	facdata)
{
    struct {
	CAPI2_facility_resp_t    a;
	char buffer[256];
    } a;
    int len;

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);

    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_FACILITY_RESP);
    PUT_WORD(    a.a.messid    ,  messid);
    PUT_DWORD(   a.a.ident     ,  ident);
    PUT_WORD(    a.a.selector  ,  selector);

    if (facdata) {
	len += facdata->length;
	memcpy( &a.a.structlen, facdata, (size_t)facdata->length+1);
    }

    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_connectb3_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_connectb3_req(int 			appl,
				   unsigned long 	plci,
				   struct userdata *	ncpi)
{
    struct {
	CAPI2_connectb3_req_t    a;
	char buffer[256];
    } a;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);

    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_CONNECTB3_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.plci      ,  plci);

    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.structlen, ncpi, (size_t)ncpi->length+1);
    }

    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_connectb3_resp						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_connectb3_resp(int 		appl,
				    unsigned short 	messid,
				    unsigned long 	ncci,
				    unsigned short	reject,
				    struct userdata *	ncpi)
{
    struct {
	CAPI2_connectb3_resp_t   a;
	char buffer[256];
    } a;
    int len;

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_CONNECTB3_RESP);
    PUT_WORD(    a.a.messid    ,  messid);
    PUT_DWORD(   a.a.ncci      ,  ncci);
    PUT_WORD(	 a.a.reject    ,  reject);

    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.structlen, ncpi, (size_t)ncpi->length+1);
    }

    PUT_WORD( a.a.len, len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_connectb3active_resp					*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_connectb3active_resp(int 			appl,
					  unsigned short  	messid,
					  unsigned long   	ncci)
{
    CAPI2_connectb3active_resp_t     a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI2_CONNECTB3ACTIVE_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_DWORD(   a.ncci      ,  ncci);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_connectb3t90active_resp					*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_connectb3t90active_resp(int 		 appl,
					     unsigned short   messid,
					     unsigned long   ncci)
{
    CAPI2_connectb3t90active_resp_t     a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI2_CONNECTB3T90ACTIVE_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_DWORD(   a.ncci      ,  ncci);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_disconnectb3_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_disconnectb3_req(int 		appl,
				      unsigned long 	ncci,
				      struct userdata *	ncpi)
{
    struct {
	CAPI2_disconnectb3_req_t a;
	char buffer[256];
    } a;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_DISCONNECTB3_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.ncci      ,  ncci);

    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.structlen, ncpi, (size_t)ncpi->length+1);
    }
    PUT_WORD(    a.a.len       ,  len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_disconnectb3_resp						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_disconnectb3_resp(int 	      	appl,
				       unsigned short 	messid,
				       unsigned long 	ncci)
{
    CAPI2_disconnectb3_resp_t        a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI2_DISCONNECTB3_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_DWORD(   a.ncci      ,  ncci);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_resetb3_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_resetb3_req(int 			appl,
				 unsigned long		ncci,
				 struct userdata	*ncpi)
{
    struct {
	CAPI2_resetb3_req_t a;
	char buffer[256];
    } a;
    int len;
    unsigned short  usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_RESETB3_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_DWORD(   a.a.ncci      ,  ncci);

    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.structlen, ncpi, (size_t)ncpi->length+1);
    }
    PUT_WORD(    a.a.len       ,  len);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_resetb3_resp						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_resetb3_resp(int 			appl,
				  unsigned short 	messid,
				  unsigned long   	ncci)
{
    struct {
	CAPI2_resetb3_resp_t a;
	char buffer[256];
    } a;

    memset(&a.a, 0, sizeof(a.a));

    PUT_WORD(    a.a.len       ,  sizeof(a.a));
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI2_RESETB3_RESP);
    PUT_WORD(    a.a.messid    ,  messid);

    PUT_DWORD(   a.a.ncci      ,  ncci);

    return capi2_put_message( appl, (char FAR *)&a);
}


/************************************************************************/
/*									*/
/*	capi2_datab3_req						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_datab3_req(int 			appl,
				unsigned long 		ncci,
				char FAR *		buffer,
				unsigned short  	len,
				unsigned short		flags,
				unsigned short		handle)
{
    CAPI2_datab3_req_t a;
    unsigned short  usMessId = get_messid();

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len,  	sizeof(a));
    PUT_WORD(    a.appl,  	appl);
    PUT_WORD(    a.PRIM_type,  	CAPI2_DATAB3_REQ);
    PUT_WORD(    a.messid,	usMessId);

    PUT_DWORD(   a.ncci,  	ncci);
    PUT_WORD(    a.datalen,  	len);
    PUT_WORD(    a.flags,  	flags);
    PUT_WORD(    a.handle,  	handle);
#ifdef __MSDOS__
    a.dataptr = buffer;
#else
    PUT_DWORD(   a.dataptr,(long)buffer);
#endif

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_datab3_resp						*/
/*      return code is the function value (see CAPI specification)      */
/*									*/
/************************************************************************/
unsigned short capi2_datab3_resp(int 		appl,
				 unsigned short messid,
				 unsigned long  ncci,
				 unsigned short handle)
{
    CAPI2_datab3_resp_t      a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len,  	sizeof(a));
    PUT_WORD(    a.appl,  	appl);
    PUT_WORD(    a.PRIM_type,  	CAPI2_DATAB3_RESP);
    PUT_WORD(    a.messid,  	messid);
    PUT_DWORD(   a.ncci,  	ncci);
    PUT_WORD(    a.handle,	handle);

    return capi2_put_message( appl, (char FAR *)&a);
}

/************************************************************************
*									*
*	capi_control_req						*
*	BinTec specific CAPI control request. Mostly used for keepalive *
*	be careful, cause its not CAPI2 standard! 			*
*									*
************************************************************************/
unsigned short capi_control_req(int		appl,
				int		contrl,
				int		type,
				struct userdata *data)
{
    int len;
    struct {
        CAPI_control_req_t a;
        char buffer[256];
    } a;
    struct userdata *dataPtr;
 
    dataPtr = (struct userdata *)&a.a.structlen;
    memset(&a, 0, sizeof(a));

    len = sizeof(a.a);
    PUT_WORD( a.a.appl,		appl);
    PUT_WORD( a.a.PRIM_type,	CAPI_CONTROL_REQ);
    PUT_WORD( a.a.messid,	0);
    PUT_WORD( a.a.contrl,	contrl);
    PUT_WORD( a.a.type,		type);

    if (data) {
	memcpy( dataPtr, data, data->length);
        len += data->length;
    }
    PUT_WORD( a.a.len,		len);
 
    return capi_put_message( appl, (char *)&a);
}
 
/************************************************************************/
/*									*/
/*	capi_control_resp						*/
/*									*/
/************************************************************************/
unsigned short capi_control_resp(int			appl,
				 int			contrl,
				 int			messid,
				 int			type,
				 struct userdata	*data)
{
    int len;
    struct {
	CAPI_control_resp_t a;
        char buffer[256];
    } a;
    struct userdata *dataPtr;
 
    dataPtr = (struct userdata *)&a.a.structlen;
    memset(&a, 0, sizeof(a));
 
    len       = sizeof(a.a);
    PUT_WORD( a.a.appl,       appl);
    PUT_WORD( a.a.PRIM_type,  CAPI_CONTROL_RESP);
    PUT_WORD( a.a.messid,     messid);
    PUT_WORD( a.a.contrl,     contrl);
    PUT_WORD( a.a.type,       type);
    if (data) {
        memcpy( dataPtr, data, data->length);
        len += data->length;
    }
    PUT_WORD( a.a.len, 	      len);

    return capi_put_message( appl, (char *)&a);
}

/************************************************************************/
/*									*/
/*	capi2_perror							*/
/*      no return code                                                  */
/*									*/
/************************************************************************/
void capi2_perror( const char *str, int info )
{

    	    fprintf( stderr, "%s: (%04x) %s\n", str ? str : "", info, capi2_strerror(info));
}


const char *capi2_strerror( int info )
{
  const char *ret = NULL;
  static struct { int info; const char *text; } *tp, tbl[] = {
    { 0x0001, "NCPI not supported by current protocol, NCPI ignored"   	},
    { 0x0002, "Flags not supported by current protocol, flags ignored" 	},
    { 0x0003, "Alert already sent by another application"		   	},
    
    { 0x1001, "Too many applications"					},
    { 0x1002, "Logical block size too small, must be at least 128 bytes"	},
    { 0x1003, "Buffer exceeds 64 kByte"					},
    { 0x1004, "Message buffer size too small, must be at least 1024 bytes"	},
    { 0x1005, "Max. number of logical connections not supported"		},
    { 0x1006, "Reserved"							},
    { 0x1007, "The message could not be accepted because of an internal busy condition" },
    { 0x1008, "OS Resource error (e.g. no memory)"				},
    { 0x1009, "COMMON-ISDN-API not installed"				},
    { 0x100a, "Controller does not support external equipment"		},
    { 0x100b, "Controller does only suport external equipment"		},
    
    { 0x1101, "Illegal application number"					},
    { 0x1102, "Illegal command or subcommand or message length less than 12 octets" },
    { 0x1103, "The message could not be accepted because of a queue full condition" },
    { 0x1104, "Queue is empty"						},
    { 0x1105, "Queue overflow, a message was lost"				},
    { 0x1106, "Unknown notification parameter"				},
    { 0x1107, "The message could not be accepted because of an internal busy condition" },
    { 0x1108, "OS resource error (e.g. no memory)"				},
    { 0x1109, "COMMON-ISDN-API not installed"				},
    { 0x110a, "Controller does not support external equipment"		},
    { 0x110b, "Controller does only suport external equipment"		},
    
    { 0x2001, "Message not supported in current state"			},
    { 0x2002, "Illegal Controller/PLCI/NCCI"				},
    { 0x2003, "Out of PLCI"							},
    { 0x2004, "Out of NCCI"							},
    { 0x2005, "Out of LISTEN"						},
    { 0x2006, "Out of FAX resources (protocol T.30)"			},
    { 0x2007, "Illegal Message parameter coding"				},
    
    { 0x3001, "B1 protocol not supported"					},
    { 0x3002, "B2 protocol not supported"					},
    { 0x3003, "B3 protocol not supported"					},
    { 0x3004, "B1 protocol parameter not supported"				},
    { 0x3005, "B2 protocol parameter not supported"				},
    { 0x3006, "B3 protocol parameter not supported"				},
    { 0x3007, "B protocol combination not supported"			},
    { 0x3008, "NCPI not supported"						},
    { 0x3009, "CIP Value unknown"						},
    { 0x300a, "Flags not supported (reserved bits)"				},
    { 0x300b, "Facility not supported"					},
    { 0x300c, "Data length not supported by currect protocol"		},
    { 0x300d, "Reset procedure not supported by current protocol"		},

    { 0x3301, "Protocol error layer 1 (broken line or B-channel removed by signalling protocol)" },
    { 0x3302, "Protocol error layer 2"					},
    { 0x3303, "Protocol error layer 3"					},
    { 0x3304, "Another application got that call"				},
    
    { 0x3311, "Connecting not successful (remote station is no FAX G3 machine)" 	},
    { 0x3312, "Connecting not successful (training error)" 				},
    { 0x3313, "Disconnected before transfer (remote station does not support transfer mode, e.g. resolution)" },
    { 0x3314, "Disconnected during transfer (remote abort)" 			},
    { 0x3315, "Disconnected during transfer (remote procedure error, e.g. unsuccessful repetition of T.30 commands)" },
    { 0x3316, "Disconnected during transfer (local tx data underrun)" 		},
    { 0x3317, "Disconnected during transfer (local rx data overflow)" 		},
    { 0x3318, "Disconnected during transfer (local abort)" 				},
    { 0x3319, "Illegal parameter coding (e.g. SFF coding error)" 			},

    { 0x3481, "Unallocated (unassigned) number"					},
    { 0x3482, "No route to specified transit network"				},
    { 0x3483, "No route to destination"						},
    { 0x3486, "Channel unacceptable"						},
    { 0x3487, "Call awarded and being delivered in an established channel"		},
    { 0x3490, "Normal call clearing"						},
    { 0x3491, "User busy"								},
    { 0x3492, "No user responding"							},
    { 0x3493, "No answer from user (user alerted)"					},
    { 0x3495, "Call rejected"							},
    { 0x3496, "Number changed"							},
    { 0x349a, "Non-selected user clearing"						},
    { 0x349b, "Destination out of order"						},
    { 0x349c, "Invalid number format"						},
    { 0x349d, "Facility rejected"							},
    { 0x349e, "Response to STATUS ENQUIRY"						},
    { 0x349f, "Normal, unspecified"							},
    { 0x34a2, "No circuit / channel available"					},
    { 0x34a6, "Network out of order"						},
    { 0x34a9, "Temporary failure"							},
    { 0x34aa, "Switching equipment congestion"					},
    { 0x34ab, "Access information discarded"					},
    { 0x34ac, "Requested circuit / channel not available"				},
    { 0x34af, "Resources unavailable, unspecified"					},
    { 0x34b1, "Quality of service unavailable"					},
    { 0x34b2, "Requested facility not subscribed"					},
    { 0x34b9, "Bearer capability not authorized"					},
    { 0x34ba, "Bearer capability not presently available"				},
    { 0x34bf, "Service or option not available, unspecified"			},
    { 0x34c1, "Bearer capability not implemented"					},
    { 0x34c2, "Channel type not implemented"					},
    { 0x34c5, "Requested facility not implemented"					},
    { 0x34c6, "Only restricted digital information bearer capability is available"	},
    { 0x34cf, "Service or option not implemented, unspecified"			},
    { 0x34d1, "Invalid call reference value"					},
    { 0x34d2, "Identified channel does not exist"					},
    { 0x34d3, "A suspended call exists, but this call identity does not"		},
    { 0x34d4, "Call identity in use"						},
    { 0x34d5, "No call suspended"							},
    { 0x34d6, "Call having the requested call identity has been cleared"		},
    { 0x34d8, "Incompatible destination"						},
    { 0x34db, "Invalid transit network selection"					},
    { 0x34df, "Invalid message, unspecified"					},
    { 0x34e0, "Mandatory information element is missing"				},
    { 0x34e1, "Message type non-existent or not implemented"			},
    { 0x34e2, "Message not compatible with call state or message type non-existent or not implemented"	},
    { 0x34e3, "Information element non-existent or not implemented"			},
    { 0x34e4, "Invalid information element contents"				},
    { 0x34e5, "Message not compatible with call state"				},
    { 0x34e6, "Recovery on timer expiry"						},
    { 0x34ef, "Protocol error, unspecified"						},
    { 0x34ff, "Interworking, unspecified"						},
    { -1,     ""									},
  };
  for (tp=tbl; tp->info != -1; tp++) {
    if (tp->info == info) {
      ret = tp->text;
      break;
    }
  }
  return(ret);
}

