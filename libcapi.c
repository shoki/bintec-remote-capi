/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: capi 1.1 library
 *      Author: heinz
 *    $RCSfile: libcapi.c,v $
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

#include "capiconf.h"

/*------------------------------------------------------------------------
 *
 * CAPI utility functions
 *
 *-----------------------------------------------------------------------*/

/************************************************************************/
/*                                                                      */
/*      get_messid                                                      */
/*                                                                      */
/************************************************************************/
/*
 * speedup such simple functions
 */
static unsigned short messid_1 = 1;
#define get_messid() (messid_1 = (++messid_1 & 0x7fff))



/************************************************************************/
/*									*/
/*	capi_listen_req							*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_listen_req(int 		appl,
			       unsigned char 	contrl,
			       unsigned long 	infomask,
			       unsigned short 	eazmask,
			       unsigned short 	simask)
{
    CAPI_listen_req_t a;
    unsigned short usMessId = get_messid();

    memset(&a, 0, sizeof(a));

    PUT_WORD(  a.len          , sizeof(a));
    PUT_WORD(  a.appl         , appl);
    PUT_WORD(  a.PRIM_type    , CAPI_LISTEN_REQ);
    PUT_WORD(  a.messid       , usMessId);
    PUT_BYTE(  a.contrl       , contrl);
    PUT_DWORD( a.info_mask    , infomask);
    PUT_WORD(  a.eaz_mask     , eazmask);
    PUT_WORD(  a.service_mask , simask);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_connect_req						*/
/*      The function value is the message id (in the sense of the CAPI) */
/*      It has to be checked against the message id in the              */
/*      confirmation.                                                   */
/*      A function value of (unsigned short)-1 means an error. The      */
/*      error code is found in capi_errno.                              */
/*									*/
/************************************************************************/
unsigned short capi_connect_req(int 		appl,
				unsigned char 	contrl,
				unsigned char 	channel,
				unsigned long 	infomask,
				unsigned char 	service,
				unsigned char	addinfo,
				unsigned char	eaz,
				struct telno * dad_telno,
				struct telno * oad_telno)
{
    int len;
    struct {
	CAPI_connect_req_t	 a;
	char buffer[180];
    } a;
    int i;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);

    PUT_WORD( a.a.appl            , appl);
    PUT_WORD( a.a.PRIM_type       , CAPI_CONNECT_REQ);
    PUT_WORD( a.a.messid          , usMessId);
    PUT_BYTE( a.a.contrl          , contrl);
    PUT_BYTE( a.a.channel         , channel);
    PUT_BYTE( a.a.dst_service     , service);
    PUT_BYTE( a.a.dst_addinfo     , addinfo);
    PUT_BYTE( a.a.src_eaz         , eaz);

    if (dad_telno) {
    	for(i= 0; i<dad_telno->length-1; i++) {
	    switch (dad_telno->no[i]) {
		case 's':
		case 'S':
		    infomask |= CAPI_ISPV;
		    break;
		case '/':
		    infomask |= CAPI_ISUBADDR;
		    break;
	    }
	}
	len += dad_telno->length;
	memcpy( &a.a.telnolen, dad_telno, (size_t)dad_telno->length+1);
    }

    if (oad_telno && oad_telno->length > 0) {
	infomask |= CAPI_IDESTINATION;
	len += oad_telno->length + 1;
	memcpy( &a.a.telnolen+dad_telno->length+1,
		oad_telno,
		(size_t)oad_telno->length+1);
    }
    PUT_WORD(  a.a.len        	   , len);
    PUT_DWORD( a.a.info_mask       , infomask);

    if (capi_put_message( appl, (char  *)&a) < 0) {
	return (unsigned short)-1;
    }
    return usMessId;
}

/************************************************************************/
/*									*/
/*	capi_getparams_req						*/
/*      The function value is the error code in the sense of the CAPI   */
/*									*/
/************************************************************************/
unsigned short capi_getparams_req(int			appl,
				  unsigned short	plci)
{
    CAPI_getparams_req_t	a;
    unsigned short 		usMessId = get_messid();

    memset(&a, 0, sizeof(a));

    PUT_WORD( a.len, 		sizeof(a));
    PUT_WORD( a.appl, 		appl);
    PUT_WORD( a.PRIM_type, 	CAPI_GETPARAMS_REQ);
    PUT_WORD( a.messid, 	usMessId);
    PUT_WORD( a.plci,      	plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_connectinfo_req						*/
/*      The function value is the return code (see CAPI specification)  */
/*									*/
/************************************************************************/
unsigned short capi_connectinfo_req(int 		appl,
				    unsigned short 	plci,
				    struct telno *	telno)
{
    int len;
    struct {
	CAPI_connectinfo_req_t a;
	char buffer[180];
    } a;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);
    PUT_WORD(    a.a.appl            ,  appl);
    PUT_WORD(    a.a.PRIM_type       ,  CAPI_CONNECTINFO_REQ);
    PUT_WORD(    a.a.messid          ,  usMessId);
    PUT_WORD(    a.a.plci            ,  plci);
    if (telno) {
	len += telno->length;
	memcpy( &a.a.telnolen, telno, (size_t)telno->length + 1);
    }
    PUT_WORD(	a.a.len ,  len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_connect_resp						*/
/*      The function value is the error code in the sense of the CAPI   */
/*									*/
/************************************************************************/
unsigned short capi_connect_resp(int 		appl,
				 unsigned short messid,
				 unsigned short plci,
				 unsigned short reject)
{
    CAPI_connect_resp_t a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.PRIM_type ,  CAPI_CONNECT_RESP);
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);
    PUT_BYTE(    a.reject    ,  reject);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_selectb2_req						*/
/*      The function value is the return code (see CAPI specification)  */
/*									*/
/************************************************************************/
unsigned short capi_selectb2_req(int 			appl,
				 unsigned short 	plci,
				 unsigned char  	proto,
				 struct userdata *	dlpd)
{
    int len;
    struct {
	 CAPI_selectb2_req_t        a;
	 char buffer[180];
    } a;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);

    PUT_WORD(    a.a.appl            ,  appl);
    PUT_WORD(    a.a.PRIM_type       ,  CAPI_SELECTB2_REQ);
    PUT_WORD(    a.a.messid          ,  usMessId);
    PUT_WORD(    a.a.plci            ,  plci);
    PUT_BYTE(    a.a.proto           ,  proto);
    if (dlpd) {
	len += dlpd->length;
	memcpy( &a.a.dlpdlen, dlpd, (size_t)dlpd->length+1);
    }
    PUT_WORD(	a.a.len ,  len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_selectb3_req						*/
/*      The function value is an error code as defined in the CAPI      */
/*      specification.                                                  */
/*									*/
/************************************************************************/
unsigned short capi_selectb3_req(int 			appl,
				 unsigned short 	plci,
				 unsigned char 		proto,
				 struct userdata *	ncpd)
{
    int len;
    struct {
	CAPI_selectb3_req_t a;
	char buffer[180];
    } a;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);
    PUT_WORD(    a.a.appl            ,  appl);
    PUT_WORD(    a.a.PRIM_type       ,  CAPI_SELECTB3_REQ);
    PUT_WORD(    a.a.messid          ,  usMessId);
    PUT_WORD(    a.a.plci            ,  plci);
    PUT_BYTE(    a.a.proto           ,  proto);

    if (ncpd) {
	len += ncpd->length;
	memcpy( &a.a.ncpdlen, ncpd, (size_t)ncpd->length+1);
    }
    PUT_WORD(    a.a.len             ,  len);

    return capi_put_message( appl, (char  *)&a);
}


/************************************************************************/
/*									*/
/*	capi_connectactive_resp						*/
/*      The function value is the error code as defined in the CAPI     */
/*      specification.                                                  */
/*									*/
/************************************************************************/
unsigned short capi_connectactive_resp(int 		appl,
				       unsigned short 	messid,
				       unsigned short 	plci)
{
    CAPI_connectactive_resp_t a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_CONNECTACTIVE_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_info_req							*/
/*      The function value is the return code (see CAPI specification)  */
/*									*/
/************************************************************************/
unsigned short capi_info_req(int 		appl,
			     unsigned short 	plci,
			     unsigned long 	infomask)
{
    CAPI_info_req_t a;
    unsigned short usMessId = get_messid();

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_INFO_REQ);
    PUT_WORD(    a.messid    ,  usMessId);
    PUT_WORD(    a.plci      ,  plci);
    PUT_DWORD(   a.info_mask ,  infomask);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_info_resp							*/
/*      The function value is the return code (as defined in the CAPI   */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_info_resp(int 		appl,
			      unsigned short 	messid,
			      unsigned short 	plci)
{
    CAPI_info_resp_t a;

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_INFO_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_disconnect_req						*/
/*      The function value is the return code (as defined in the CAPI   */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_disconnect_req(int 			appl,
				   unsigned short 	plci,
				   unsigned char 	cause)
{
    CAPI_disconnect_req_t a;
    unsigned short usMessId = get_messid();

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.PRIM_type ,  CAPI_DISCONNECT_REQ);
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.messid    ,  usMessId);
    PUT_WORD(    a.plci      ,  plci);
    PUT_BYTE(    a.cause     ,  cause);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_disconnect_resp						*/
/*      The CAPI error code appears as the function value (see CAPI     */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_disconnect_resp(int 		appl,
				    unsigned short 	messid,
				    unsigned short 	plci)
{
    CAPI_disconnect_resp_t a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.PRIM_type ,  CAPI_DISCONNECT_RESP);
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_data_req							*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_data_req(int 		appl,
			     unsigned short 	plci,
			     struct userdata *	data)
{
    int len;
    struct {
	CAPI_data_req_t     	a;
	char 			buffer[180];
    } a;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);
    PUT_WORD(    a.a.appl       ,  appl);
    PUT_WORD(    a.a.PRIM_type  ,  CAPI_DATA_REQ);
    PUT_WORD(    a.a.messid     ,  usMessId);
    PUT_WORD(    a.a.plci       ,  plci);

    if (data) {
	len += data->length;
	memcpy( &a.a.structlen, data, (size_t)data->length+1);
    }
    PUT_WORD(    a.a.len        ,  len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_data_resp							*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_data_resp(int 		appl,
			      unsigned short 	messid,
			      unsigned short 	plci)
{
    CAPI_data_resp_t a;

    memset( &a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_DATA_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}


/************************************************************************/
/*									*/
/*	capi_listenb3_req						*/
/*      The CAPI error code appears as the function value (see CAPI     */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_listenb3_req(int 		appl,
				 unsigned short plci)
{
    CAPI_listenb3_req_t a;
    unsigned short usMessId = get_messid();

    memset( &a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_LISTENB3_REQ);
    PUT_WORD(    a.messid    ,  usMessId);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_connectb3_req						*/
/*      The function value is the return code (see CAPI specification)  */
/*									*/
/************************************************************************/
unsigned short capi_connectb3_req(int 			appl,
				  unsigned short 	plci,
				  struct userdata *	ncpi)
{
    int len;
    struct {
	CAPI_connectb3_req_t    a;
	char buffer[180];
    } a;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI_CONNECTB3_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_WORD(    a.a.plci      ,  plci);

    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.ncpilen, ncpi, (size_t)ncpi->length+1);
    }
    PUT_WORD(	a.a.len ,  len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_connectb3_resp						*/
/*      The function value is the return code (see CAPI specification)  */
/*									*/
/************************************************************************/
unsigned short capi_connectb3_resp(int 			appl,
				   unsigned short 	messid,
				   unsigned short 	ncci,
				   unsigned short 	reject,
				   struct userdata *	ncpi)
{
    int len;
    struct {
	CAPI_connectb3_resp_t   a	;
	char buffer[180]		;
    } a;

    memset(&a.a, 0, sizeof(a.a));
    len = sizeof(a.a);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI_CONNECTB3_RESP);
    PUT_WORD(    a.a.messid    ,  messid);
    PUT_WORD(    a.a.ncci      ,  ncci);
    PUT_BYTE(    a.a.reject    ,  reject);
    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.ncpilen, ncpi, (size_t)ncpi->length+1);
    }
    PUT_WORD(    a.a.len       ,  len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_connectb3active_resp					*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_connectb3active_resp(int 		 appl,
					 unsigned short  messid,
					 unsigned short  ncci)
{
    CAPI_connectb3active_resp_t     a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_CONNECTB3ACTIVE_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.ncci      ,  ncci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_disconnectb3_req						*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_disconnectb3_req(int 		appl,
				     unsigned short 	ncci,
				     struct userdata *	ncpi)
{
    int len;
    struct {
	CAPI_disconnectb3_req_t a	;
	char buffer[180]		;
    } a;
    unsigned short usMessId = get_messid();

    memset(&a.a, 0, sizeof(a.a));

    len = sizeof(a.a);
    PUT_WORD(    a.a.appl      ,  appl);
    PUT_WORD(    a.a.PRIM_type ,  CAPI_DISCONNECTB3_REQ);
    PUT_WORD(    a.a.messid    ,  usMessId);
    PUT_WORD(    a.a.ncci      ,  ncci);
    if (ncpi) {
	len += ncpi->length;
	memcpy( &a.a.ncpilen, ncpi, (size_t)ncpi->length+1);
    }
    PUT_WORD(    a.a.len       ,  len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_resetb3_req						*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_resetb3_req(int 		appl,
				unsigned short 	ncci)
{
    CAPI_resetb3_req_t a;
    unsigned short usMessId = get_messid();

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_RESETB3_REQ);
    PUT_WORD(    a.messid    ,  usMessId);
    PUT_WORD(    a.ncci      ,  ncci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_resetb3_resp						*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_resetb3_resp(int 		appl,
				 unsigned short messid,
				 unsigned short ncci)
{
    CAPI_resetb3_resp_t a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_RESETB3_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.ncci      ,  ncci);

    return capi_put_message( appl, (char  *)&a);
}


/************************************************************************/
/*									*/
/*	capi_disconnectb3_resp						*/
/*      The function value is the error code in the sense of the CAPI   */
/*      (see CAPI specification).                                       */
/*									*/
/************************************************************************/
unsigned short capi_disconnectb3_resp(int 	      	appl,
				      unsigned short 	messid,
				      unsigned short 	ncci)
{
    CAPI_disconnectb3_resp_t        a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_DISCONNECTB3_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.ncci      ,  ncci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_datab3_req							*/
/*      The function value is the error code in the sense of the CAPI   */
/*      (see CAPI specification).                                       */
/*									*/
/************************************************************************/
unsigned short capi_datab3_req(int 		appl,
			       unsigned short 	ncci,
			       char  *		buffer,
			       unsigned short  	len,
			       unsigned short	flags,
			       unsigned char	blknum)
{
    CAPI_datab3_req_t a;
    unsigned short usMessId = get_messid();

    memset(&a, 0, sizeof(a));

    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_DATAB3_REQ);
    PUT_WORD(    a.messid    ,  usMessId);

    PUT_WORD(    a.ncci      ,  ncci);
    PUT_WORD(    a.datalen   ,  len);
    PUT_WORD(    a.flags     ,  flags);
    PUT_BYTE(    a.blknum    ,  blknum);
    PUT_DWORD(   a.data	,  (long)buffer);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_datab3_resp						*/
/*      The function value is the CAPI error code (see CAPI             */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_datab3_resp(int 		appl,
				unsigned short  messid,
				unsigned short  ncci,
				unsigned char   blknum)
{
    CAPI_datab3_resp_t      a;

    memset(&a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_DATAB3_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.ncci      ,  ncci);
    PUT_BYTE(    a.blknum    ,  blknum);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_handset_resp						*/
/*      Again the function value is the CAPI error code (see CAPI       */
/*      specification).                                                 */
/*									*/
/************************************************************************/
unsigned short capi_handset_resp(int 		appl,
				 unsigned short messid,
				 unsigned short plci)
{
    CAPI_handset_resp_t a;

    memset( &a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_HANDSET_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_dtmf_req							*/
/*      Guess the meaning of the function value: its the CAPI error     */
/*									*/
/************************************************************************/
unsigned short capi_dtmf_req(int 		appl,
			     unsigned short 	plci,
			     struct userdata *	data)
{
    int len;
    struct {
	CAPI_dtmf_req_t a;
	char buffer[180];
    } a;
    unsigned short usMessId = get_messid();

    memset( &a.a, 0, sizeof(a.a));
    len = sizeof(a.a);
    PUT_WORD( a.a.appl     , appl);
    PUT_WORD( a.a.PRIM_type, CAPI_DTMF_REQ);
    PUT_WORD( a.a.messid   , usMessId);
    PUT_WORD( a.a.plci     , plci);
    if (data) {
	memcpy( &a.a.dtmflen, data, (size_t)data->length+1);
	len += data->length+1;
    }
    PUT_WORD( a.a.len      , len);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_dtmf_resp							*/
/*      The meaning of the function value is the CAPI error code        */
/*									*/
/************************************************************************/
unsigned short capi_dtmf_resp(int 		appl,
			      unsigned short 	messid,
			      unsigned short 	plci)
{
    CAPI_dtmf_resp_t a;

    memset( &a, 0, sizeof(a));
    PUT_WORD(    a.len       ,  sizeof(a));
    PUT_WORD(    a.appl      ,  appl);
    PUT_WORD(    a.PRIM_type ,  CAPI_DTMF_RESP);
    PUT_WORD(    a.messid    ,  messid);
    PUT_WORD(    a.plci      ,  plci);

    return capi_put_message( appl, (char  *)&a);
}

/************************************************************************/
/*									*/
/*	capi_msg							*/
/*									*/
/************************************************************************/
const char *capi_msg(union CAPI_primitives *a)
{
    static struct { long PRIM_type; const char *text; } *msgp, apimsg[] = {
	{ CAPI_CONNECT_REQ,              "CAPI_CONNECT_REQ"               },
	{ CAPI_CONNECT_CONF,             "CAPI_CONNECT_CONF"              },
	{ CAPI_CONNECT_IND,              "CAPI_CONNECT_IND"               },
	{ CAPI_CONNECT_RESP,             "CAPI_CONNECT_RESP"              },
	{ CAPI_CONNECTINFO_REQ,          "CAPI_CONNECTINFO_REQ"           },
	{ CAPI_CONNECTINFO_CONF,         "CAPI_CONNECTINFO_CONF"          },
	{ CAPI_CONNECTACTIVE_IND,        "CAPI_CONNECTACTIVE_IND"         },
	{ CAPI_CONNECTACTIVE_RESP,       "CAPI_CONNECTACTIVE_RESP"        },
	{ CAPI_DISCONNECT_REQ,           "CAPI_DISCONNECT_REQ"            },
	{ CAPI_DISCONNECT_CONF,          "CAPI_DISCONNECT_CONF"           },
	{ CAPI_DISCONNECT_IND,           "CAPI_DISCONNECT_IND"            },
	{ CAPI_DISCONNECT_RESP,          "CAPI_DISCONNECT_RESP"           },
	{ CAPI_LISTEN_REQ,               "CAPI_LISTEN_REQ"                },
	{ CAPI_LISTEN_CONF,              "CAPI_LISTEN_CONF"               },
	{ CAPI_GETPARAMS_REQ,            "CAPI_GETPARAMS_REQ"             },
	{ CAPI_GETPARAMS_CONF,           "CAPI_GETPARAMS_CONF"            },
	{ CAPI_INFO_REQ ,                "CAPI_INFO_REQ"                  },
	{ CAPI_INFO_CONF,                "CAPI_INFO_CONF"                 },
	{ CAPI_INFO_IND,                 "CAPI_INFO_IND"                  },
	{ CAPI_INFO_RESP,                "CAPI_INFO_RESP"                 },
	{ CAPI_DATA_REQ,                 "CAPI_DATA_REQ"                  },
	{ CAPI_DATA_CONF,                "CAPI_DATA_CONF"                 },
	{ CAPI_DATA_IND,                 "CAPI_DATA_IND"                  },
	{ CAPI_DATA_RESP,                "CAPI_DATA_RESP"                 },
	{ CAPI_SELECTB2_REQ,             "CAPI_SELECTB2_REQ"              },
	{ CAPI_SELECTB2_CONF,            "CAPI_SELECTB2_CONF"             },
	{ CAPI_SELECTB3_REQ,             "CAPI_SELECTB3_REQ"              },
	{ CAPI_SELECTB3_CONF,            "CAPI_SELECTB3_CONF"             },
	{ CAPI_LISTENB3_REQ,             "CAPI_LISTENB3_REQ"              },
	{ CAPI_LISTENB3_CONF,            "CAPI_LISTENB3_CONF"             },
	{ CAPI_CONNECTB3_REQ,            "CAPI_CONNECTB3_REQ"             },
	{ CAPI_CONNECTB3_CONF,           "CAPI_CONNECTB3_CONF"            },
	{ CAPI_CONNECTB3_IND,            "CAPI_CONNECTB3_IND"             },
	{ CAPI_CONNECTB3_RESP,           "CAPI_CONNECTB3_RESP"            },
	{ CAPI_CONNECTB3ACTIVE_IND,      "CAPI_CONNECTB3ACTIVE_IND"       },
	{ CAPI_CONNECTB3ACTIVE_RESP,     "CAPI_CONNECTB3ACTIVE_RESP"      },
	{ CAPI_DISCONNECTB3_REQ,         "CAPI_DISCONNECTB3_REQ"          },
	{ CAPI_DISCONNECTB3_CONF,        "CAPI_DISCONNECTB3_CONF"         },
	{ CAPI_DISCONNECTB3_IND,         "CAPI_DISCONNECTB3_IND"          },
	{ CAPI_DISCONNECTB3_RESP,        "CAPI_DISCONNECTB3_RESP"         },
	{ CAPI_GETB3PARAMS_REQ,          "CAPI_GETB3PARAMS_REQ"           },
	{ CAPI_GETB3PARAMS_CONF,         "CAPI_GETB3PARAMS_CONF"          },
	{ CAPI_DATAB3_REQ,               "CAPI_DATAB3_REQ"                },
	{ CAPI_DATAB3_CONF,              "CAPI_DATAB3_CONF"               },
	{ CAPI_DATAB3_IND,               "CAPI_DATAB3_IND"                },
	{ CAPI_DATAB3_RESP,              "CAPI_DATAB3_RESP"               },
	{ CAPI_RESETB3_REQ,              "CAPI_RESETB3_REQ"               },
	{ CAPI_RESETB3_CONF,             "CAPI_RESETB3_CONF"              },
	{ CAPI_RESETB3_IND,              "CAPI_RESETB3_IND"               },
	{ CAPI_RESETB3_RESP,             "CAPI_RESETB3_RESP"              },
	{ CAPI_HANDSET_IND,              "CAPI_HANDSET_IND"               },
	{ CAPI_HANDSET_RESP,             "CAPI_HANDSET_RESP"              },
	{ CAPI_DTMF_REQ,                 "CAPI_DTMF_REQ" 	          },
	{ CAPI_DTMF_CONF,                "CAPI_DTMF_CONF"                 },
	{ CAPI_DTMF_IND,                 "CAPI_DTMF_IND" 	          },
	{ CAPI_DTMF_RESP,                "CAPI_DTMF_RESP"                 },

        { CAPI2_CONNECT_REQ,             "CAPI2_CONNECT_REQ"              },
        { CAPI2_CONNECT_CONF,            "CAPI2_CONNECT_CONF"             },
        { CAPI2_CONNECT_IND,             "CAPI2_CONNECT_IND"              },
        { CAPI2_CONNECT_RESP,            "CAPI2_CONNECT_RESP"             },
        { CAPI2_ALERT_REQ,               "CAPI2_ALERT_REQ"                },
        { CAPI2_ALERT_CONF,              "CAPI2_ALERT_CONF"               },
        { CAPI2_LISTEN_REQ,              "CAPI2_LISTEN_REQ"               },
	{ CAPI2_LISTEN_CONF,             "CAPI2_LISTEN_CONF"              },
	{ CAPI2_DISCONNECT_REQ,          "CAPI2_DISCONNECT_REQ"           },
	{ CAPI2_DISCONNECT_CONF,         "CAPI2_DISCONNECT_CONF"          },
	{ CAPI2_DISCONNECT_IND,          "CAPI2_DISCONNECT_IND"           },
	{ CAPI2_DISCONNECT_RESP,         "CAPI2_DISCONNECT_RESP"          },
	{ CAPI2_INFO_REQ ,               "CAPI2_INFO_REQ"                 },
	{ CAPI2_INFO_CONF,               "CAPI2_INFO_CONF"                },
	{ CAPI2_INFO_IND,                "CAPI2_INFO_IND"                 },
	{ CAPI2_INFO_RESP,               "CAPI2_INFO_RESP"                },
	{ CAPI2_DATAB3_REQ,              "CAPI2_DATAB3_REQ"               },
	{ CAPI2_DATAB3_CONF,             "CAPI2_DATAB3_CONF"              },
	{ CAPI2_DATAB3_IND,              "CAPI2_DATAB3_IND"               },
	{ CAPI2_DATAB3_RESP,             "CAPI2_DATAB3_RESP"              },
	{ CAPI2_DISCONNECTB3_REQ,        "CAPI2_DISCONNECTB3_REQ"         },
	{ CAPI2_DISCONNECTB3_CONF,       "CAPI2_DISCONNECTB3_CONF"        },
	{ CAPI2_DISCONNECTB3_IND,        "CAPI2_DISCONNECTB3_IND"         },
	{ CAPI2_DISCONNECTB3_RESP,       "CAPI2_DISCONNECTB3_RESP"        },
	{ CAPI2_SELECTB_REQ,             "CAPI2_SELECTB_REQ"              },
	{ CAPI2_SELECTB_CONF,            "CAPI2_SELECTB_CONF"             },
	{ CAPI2_CONNECTB3_REQ,           "CAPI2_CONNECTB3_REQ"            },
	{ CAPI2_CONNECTB3_CONF,          "CAPI2_CONNECTB3_CONF"           },
	{ CAPI2_CONNECTB3_IND,           "CAPI2_CONNECTB3_IND"            },
	{ CAPI2_CONNECTB3_RESP,          "CAPI2_CONNECTB3_RESP"           },
	{ CAPI2_CONNECTACTIVE_IND,       "CAPI2_CONNECTACTIVE_IND"        },
	{ CAPI2_CONNECTACTIVE_RESP,      "CAPI2_CONNECTACTIVE_RESP"       },
	{ CAPI2_CONNECTB3ACTIVE_IND,     "CAPI2_CONNECTB3ACTIVE_IND"      },
	{ CAPI2_CONNECTB3ACTIVE_RESP,    "CAPI2_CONNECTB3ACTIVE_RESP"     },
	{ CAPI2_CONNECTB3T90ACTIVE_IND,  "CAPI2_CONNECTB3T90ACTIVE_IND"   },
	{ CAPI2_CONNECTB3T90ACTIVE_RESP, "CAPI2_CONNECTB3T90ACTIVE_RESP"  },
	{ CAPI2_FACILITY_REQ,            "CAPI2_FACILITY_REQ"             },
	{ CAPI2_FACILITY_CONF,           "CAPI2_FACILITY_CONF"            },
	{ CAPI2_FACILITY_IND,            "CAPI2_FACILITY_IND"             },
	{ CAPI2_FACILITY_RESP,           "CAPI2_FACILITY_RESP"            },
	{ CAPI2_RESETB3_REQ,             "CAPI2_RESETB3_REQ"              },
	{ CAPI2_RESETB3_CONF,            "CAPI2_RESETB3_CONF"             },
	{ CAPI2_RESETB3_IND,             "CAPI2_RESETB3_IND"              },
	{ CAPI2_RESETB3_RESP,            "CAPI2_RESETB3_RESP"             },


	{ CAPI_REGISTER_REQ,		 "BINTEC_REGISTER_REQ"		  },
	{ CAPI_REGISTER_CONF,		 "BINTEC_REGISTER_CONF"		  },
	{ CAPI_RELEASE_REQ,		 "BINTEC_RELEASE_REQ"		  },
	{ CAPI_RELEASE_CONF,		 "BINTEC_RELEASE_CONF"		  },
	{ CAPI_GETMANUFACT_REQ,		 "BINTEC_GETMANUFACT_REQ"	  },
	{ CAPI_GETMANUFACT_CONF,	 "BINTEC_GETMANUFACT_CONF"	  },
	{ CAPI_GETVERSION_REQ,		 "BINTEC_GETVERSION_REQ"	  },
	{ CAPI_GETVERSION_CONF,		 "BINTEC_GETVERSION_CONF"	  },
	{ CAPI_GETSERIAL_REQ,		 "BINTEC_GETSERIAL_REQ"		  },
	{ CAPI_GETSERIAL_CONF,		 "BINTEC_GETSERIAL_CONF"	  },
	{ CAPI_GETPROFILE_REQ,		 "BINTEC_GETPROFILE_REQ"	  },
	{ CAPI_GETPROFILE_CONF,		 "BINTEC_GETPROFILE_CONF"	  },

	{ -1,                           "" }
    };

    for (msgp = apimsg;
	 msgp->PRIM_type != -1
		&& msgp->PRIM_type != GET_WORD(a->sheader.PRIM_type);
	 msgp++);

    return msgp->text;
}

/************************************************************************/
/*									*/
/*	capi_perror							*/
/*									*/
/************************************************************************/
void capi_perror( str, info )
const char *str;
int info;
{
    fprintf( stderr, "%s: %s\n", str ? str : "", capi_strerror(info));
}

const char *capi_strerror(int info )
{
    static struct { int info; const char *text; } *tp, tbl[] = {
	{ 0x1001, "Application registration error"                        },
	{ 0x1002, "Wrong application id"                                  },
	{ 0x1003, "Message error"                                         },
	{ 0x1004, "Wrong CAPI command"                                    },
	{ 0x1005, "Message queue full"                                    },
	{ 0x1006, "Message queue empty"                                   },
	{ 0x1007, "Messages lost"                                         },
	{ 0x1008, "Error during deinstallation"                           },

	{ 0x2001, "Wrong controller"                                      },
	{ 0x2002, "Wrong PLCI"                                            },
	{ 0x2003, "Wrong NCCI"                                            },
	{ 0x2004, "Wrong type"                                            },

	{ 0x3101, "B-channel incorrectly coded"                           },
	{ 0x3102, "Info mask incorrectly coded"                           },
	{ 0x3103, "Service SI mask incorrectly coded"                     },
	{ 0x3104, "Service EAZ mask incorrectly coded"                    },
	{ 0x3105, "B2 protocol incorrect"                                 },
	{ 0x3106, "DLPD incorrect"                                        },
	{ 0x3107, "B3 protocol incorrect"                                 },
	{ 0x3108, "NCPD incorrect"                                        },
	{ 0x3109, "NCPI incorrect"                                        },
	{ 0x310a, "Flags incorrectly coded"                               },

	{ 0x3201, "Controller error"                                      },
	{ 0x3202, "Conflict between registrations"                        },
	{ 0x3203, "Function is not supported"                             },
	{ 0x3204, "PLCI not activ"                                        },
	{ 0x3205, "NCCI not activ"                                        },
	{ 0x3206, "B2 protocol not supported"                             },
	{ 0x3207, "Change of B2 protocol not possible in this state"      },
	{ 0x3208, "B3 protocol not supported"                             },
	{ 0x3209, "Change of B3 protocol not possible in this state"      },
	{ 0x320a, "Parameters used not supported in DLPD"                 },
	{ 0x320b, "Parameters used not supported in NCPD"                 },
	{ 0x320c, "Parameters used not supported in NCPI"                 },
	{ 0x320d, "Data length not supported"                             },
	{ 0x320e, "DTMF number unknown" 	                          },

	{ 0x3301, "Error on setup of D-channel layer 1"                   },
	{ 0x3302, "Error on setup of D-channel layer 2"                   },
	{ 0x3303, "Error on setup of B-channel layer 1"                   },
	{ 0x3304, "Error on setup of B-channel layer 2"                   },
	{ 0x3305, "Abort D-channel layer 1"                               },
	{ 0x3306, "Abort D-channel layer 2"                               },
	{ 0x3307, "Abort D-channel layer 3"                               },
	{ 0x3308, "Abort B-channel layer 1"                               },
	{ 0x3309, "Abort B-channel layer 2"                               },
	{ 0x330a, "Abort B-channel layer 3"                               },
	{ 0x330b, "Reestablish B-channel layer 2"                         },
	{ 0x330c, "Reestablish B-channel layer 3"                         },

	{ 0x3400, "Normal call clearing"                                  },
	{ 0x3480, "Normal call clearing"                                  },
	{ 0x3481, "Invalid call reference value"                          },
	{ 0x3483, "Bearer service not implemented"                        },
	{ 0x3487, "Call identity does not exist"                          },
	{ 0x3488, "Call identity in use"                                  },
	{ 0x348a, "No channel available"                                  },
	{ 0x3490, "Requested facility not implemented"                    },
	{ 0x3491, "Requested facility not subscribed"                     },
	{ 0x34a0, "Outgoing calls barred"                                 },
	{ 0x34a1, "User Busy"                                             },
	{ 0x34a2, "CUG compare failed"                                    },
	{ 0x34a5, "As SPV not allowed"                                    },
	{ 0x34b0, "Reverse charging not allowed at origination"           },
	{ 0x34b1, "Reverse charging not allowed at destination"           },
	{ 0x34b2, "Reverse charging rejected"                             },
	{ 0x34b5, "Destination not obtainable"                            },
	{ 0x34b8, "Number changed"                                        },
	{ 0x34b9, "Out of order"                                          },
	{ 0x34ba, "User not responding"                                   },
	{ 0x34bb, "User access busy"                                      },
	{ 0x34bd, "Incoming calls barred"                                 },
	{ 0x34be, "Call rejected"                                         },
	{ 0x34d9, "Network congestion"                                    },
	{ 0x34da, "Remote user initiated"                                 },
	{ 0x34f0, "Local procedure error"                                 },
	{ 0x34f1, "Remote procedure error"                                },
	{ 0x34f2, "Remote user suspended"                                 },
	{ 0x34f3, "Remote user resumed"                                   },
	{ 0x34ff, "User info discarded locally"                           },
	{ 0x34a3, "Non existant CUG"                                      },

	{ 0x4001, "Remote station is not a fax G3 machine"		  },
	{ 0x4002, "Local fax modul busy"				  },
	{ 0x4003, "Disconnected during transfer (remote abort)"		  },
	{ 0x4004, "Disconnected before transfer (training error)"	  },
	{ 0x4005, "Disconnected during transfer (local tx data underrun)" },
	{ 0x4006, "Fax module temporary disabled"			  },
	{ 0x4007, "Local disconnect (local abort)"			  },
	{ 0x4008, "Disconnect during transfer (remote procedure error)"	  },
	{ 0x4009, "Remote disconnect (remote abort)"			  },
	{ 0x400a, "Line disconnect during transfer"			  },
	{ 0x400b, "Disconnect before transfer (remote does not support transfer mode)" },
	{ 0x400c, "Local disconnect (SFF coding error)"			  },
	{ -1,     "Undefined error code"                                  },
    };

    for (tp=tbl; tp->info != -1; tp++) {
	if (tp->info == info) {
	    break;
	}
    }
    return(tp->text);
}
