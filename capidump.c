/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: capi packet decoder for traceing 
 *      Author: oliver
 *    $RCSfile: capidump.c,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: library
 *    Products: capitrace	(remote application for UNIX-Hosts)
 *    		DimeTools	(remote Windows application)
 *  Interfaces: -
 *   Libraries: -
 *    Switches: -
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/

/* includes */

#include <capiconf.h>

#include "capidump.h"

/* prototypes for CAPI 1.1 message parsing */

#ifndef PROTO	
#ifdef __STDC__
#define PROTO(x) x
#else
#define PROTO(x) ()
#endif
#endif

int 	si_out PROTO(( unsigned char ));
int 	channel_out PROTO((unsigned char));
int 	b2_out PROTO(( unsigned char ));
int 	b3_out PROTO(( unsigned char ));
int 	imask_out PROTO(( unsigned long ));
int 	smask_out PROTO(( unsigned short ));
int 	emask_out PROTO(( unsigned long ));
void	dump_info PROTO(( unsigned short, int));
CONST char *getErrorText PROTO(( unsigned long));
CONST char *capi_msgOut PROTO(( union CAPI_primitives *));

/* type declarations for CAPI 2.0 data structures not in capidef.h */

struct telno2 {
    cBYTE	length;
    cBYTE	type;
    cBYTE	indicator;
    cBYTE	no[32];
};

/* prototypes for CAPI 2.0 message parsing */

void	dump_c2info PROTO(( unsigned short, int));
void	dump_c2reason PROTO(( unsigned short, int));
void	dump_c2reasonB3 PROTO(( unsigned short, int));

struct userdata	*get_struct PROTO(( union CAPI_primitives *, char *));
struct userdata	*skip_struct PROTO(( union CAPI_primitives *, char *));
int		check_struct PROTO(( struct userdata *, char *,
			int, CONST char *, int, int, int));
int		check_len PROTO(( int, int, int));

void	dump_c2additional_info PROTO(( union CAPI_primitives *,
					struct addinfo *, char *, int));
void	dump_c2cip_value PROTO(( unsigned short, int));
void	dump_c2telno_1byte PROTO(( struct telno *, char *, CONST char *, int));
void	dump_c2telno_2byte PROTO(( struct telno2 *, char *, CONST char *, int));
void	dump_c2user_data PROTO(( struct userdata *, char *,
		int, CONST char *, int, int, int));
void	dump_c2b_protocol PROTO(( union CAPI_primitives *, struct bprotocol *,
		int));
void	dump_c2b_name PROTO(( int, int, int));
void	dump_c2ncpi_x25 PROTO(( struct ncpi2_x25 *, char *, int));
void	dump_c2flags PROTO(( unsigned int));
void	dump_c2reject PROTO(( unsigned short, int));



/************************************************************************/
/*									*/
/*	dump_capimsg							*/
/*									*/
/************************************************************************/
int dump_capimsg( a, flag, inout, timestamp, seqcnt) 
union CAPI_primitives	*a;
unsigned long		flag;
unsigned long		inout;
unsigned long		timestamp;
unsigned long		seqcnt;
{
    static unsigned long localSeqCnt = 2;
    unsigned short	msglen;

    char		*msgend;
    struct telno	*tn;
    struct telno	*oadtn;
    struct dlpd		*dlpd;
    struct dlpd_v110	*dlpd_v110;
    struct userdata	*ncpd;
    struct ncpd_x25	*ncpd_x25;
    struct ncpd_fax	*ncpd_t30;
    struct ncpi_fax	*ncpi_t30;
    struct userdata	*data;
    struct userdata	*station_id;
    struct userdata	*head_line;
    struct userdata 	*received_id;
    struct userdata	*bad_pages;

    unsigned short	primType;
    enum { BINTEC_TYPE, CAPI11_TYPE, CAPI20_TYPE} msgType;
    int			messid;

/* CAPI 2.0 data structures */

    struct telno	*calledPartyNumberPtr, *calledPartySubaddressPtr,
			*callingPartySubaddressPtr, *connectedSubaddressPtr;
    struct telno2	*callingPartyNumberPtr, *connectedNumberPtr;
    struct bprotocol	*bProtocolPtr;
    struct addinfo	*additionalInfoPtr;
    struct ncpi2_x25	*ncpiPtr;
    struct userdata	*bcPtr, *llcPtr, *hlcPtr, *dataPtr;

/* start of message print out */

    msglen = GET_LEN( a);
    msgend = (char *)a + msglen;

    localSeqCnt++;

/* test sequence count for lost messages */

    if ( seqcnt > localSeqCnt )
    {
	printf("ATTENTION: %lu trace message%s been lost!\n\n",
		seqcnt - localSeqCnt,
		seqcnt - localSeqCnt == 1 ? " has" : "s have");
	localSeqCnt = seqcnt;
    };

/* test for valid message length */

    if ( msglen < 8 ) /* not even a complete header! */
    {
    	printf("%07lu.%03lu",    timestamp/1000, timestamp%1000);
	printf(" %c",            inout ? 'X' : 'R');
	printf(" INVALID MESSAGE LENGTH!      ");
	printf(" #%lu",          seqcnt);
	printf(" [%04u]",        msglen);
	printf(" (has to be >= 8)\n\n");
	goto dump_capimsg_error_dump;
    };
    /* from now on there should be a correct header */
    
    msgType = BINTEC_TYPE;
    primType = GET_PRIMTYPE( a);
    if ( primType <= 0x0400 )
	msgType = CAPI11_TYPE;
    else if ( primType >= 0x8000 && primType <= 0x8400 )
     	msgType = CAPI20_TYPE;

    if ( msglen > 180 && msgType == CAPI11_TYPE ) {
    	printf("%07lu.%03lu",    timestamp/1000, timestamp%1000);
	printf(" %c",            inout ? 'X' : 'R');
	printf(" INVALID MESSAGE LENGTH!      ");
	printf(" #%lu",          seqcnt);
	printf(" [%04u]",        msglen);
	printf(" (has to be <= 180)\n\n");
	goto dump_capimsg_error_dump;
    };

/* always print info line including standard CAPI header */

    printf("%07lu.%03lu",    	timestamp/1000, timestamp%1000);
    printf(" %c",            	inout ? 'X' : 'R');
    printf(" %-29s (0x%04x)",	capi_msgOut( a), primType);
    printf(" #%lu",          	seqcnt);
    printf(" [%04u]",        	msglen);
    printf(" ID=%u",       	GET_APPL( a));

    messid	=	GET_MESSID( a);
    messid	= (messid&0x8000)?
			-(messid&0x7fff):
			 (messid&0x7fff);
    printf(" no(%d)",		messid);
/* if flag signals short output, print connection identifier */
    if ( flag & FL_SHORTOUT )
    {
	if ( msgType == CAPI20_TYPE && msglen >= 12 )
	    printf(" ident=0x%08lx", GET_c2IDENT( a));
	else if ( msgType == CAPI11_TYPE && msglen >=10 )
	    printf(" ident=0x%04x", GET_IDENT( a));
    };
    printf("\n");

/* if flag signals hexadecimal output, print it */

    if ( flag & FL_HEXOUT )
    {
	capi_hexdump_d( (char *) a, msglen, 0x10, 2, 6);
    };

/* if flag signals long output, print all message parameters */

    if ( flag & FL_LONGOUT )
    {
	switch ( primType )
	{

/*************************************************************************/
/*	CAPI 1.1 messages						 */
/*************************************************************************/

	    case CAPI_CONNECT_REQ:
		if ( ! check_len( msglen, 18, -1) ) break;
		printf("Controller           : %d\n", a->connect_req.contrl);
		printf("B-channel            : 0x%02x ", 
			a->connect_req.channel);
			channel_out( a->connect_req.channel);
			printf("\n");
		printf("Info Mask            : 0x%08lx ", 
			GET_DWORD(a->connect_req.info_mask));
			imask_out( GET_DWORD(a->connect_req.info_mask));
			printf("\n");
		printf("Outgoing service     : %d ", 
			a->connect_req.dst_service);
			si_out( a->connect_req.dst_service);
			printf("\n");
		printf("Outgoing service add.: %d\n", 
			a->connect_req.dst_addinfo);
		printf("Source EAZ           : %c\n",
			isprint(a->connect_req.src_eaz) ?
			a->connect_req.src_eaz : '-');
		tn = (struct telno *)
			get_struct( a, &(a->connect_req.telnolen));
		dump_c2telno_1byte( tn, msgend, "Destination Address", 21);
		if ( msglen > sizeof( CAPI_connect_req_t) + tn->length )
		{
		    if ( GET_DWORD( a->connect_req.info_mask) &
			 CAPI_IDESTINATION )
		    {
			oadtn = (struct telno *) skip_struct( a, (char *) tn);
			dump_c2telno_1byte( oadtn, msgend,
				"Origination Address", 21);
		    };
		};
		break;
	    case CAPI_CONNECT_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->connect_conf.plci));
		dump_info( GET_WORD( a->connect_conf.info), 4);
		break;
	    case CAPI_CONNECT_IND:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("PLCI                  : 0x%04x\n",
			GET_WORD( a->connect_ind.plci));
		printf("Controller            : %d\n", a->connect_ind.contrl);
		printf("Requested service     : %d ",
			a->connect_ind.dst_service);
			si_out( a->connect_ind.dst_service);
			printf("\n");
		printf("Requested service add.: %d\n",
			a->connect_ind.dst_addinfo);
		printf("Requested EAZ         : %c\n", 
			isprint(a->connect_ind.dst_eaz) ? 
			a->connect_ind.dst_eaz : '-');
		tn = (struct telno *)
			get_struct( a, &(a->connect_ind.telnolen));
		dump_c2telno_1byte( tn, msgend, "Caller address", 22);
		break;
	    case CAPI_CONNECT_RESP:
		if ( ! check_len( msglen, 11, 11) ) break;
		printf("PLCI  : 0x%04x\n", GET_WORD( a->connect_resp.plci));
		printf("Reject: %d\n", a->connect_resp.reject);
		break;
	    case CAPI_CONNECTINFO_REQ:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("PLCI               : 0x%04x\n",
			GET_WORD( a->connectinfo_req.plci));
		tn = (struct telno *)
			get_struct( a, &(a->connectinfo_req.telnolen));
		dump_c2telno_1byte( tn, msgend, "Destination address", 19);
		break;
	    case CAPI_CONNECTINFO_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->connectinfo_conf.plci));
		dump_info( GET_WORD( a->connectinfo_conf.info), 4);
		break;
	    case CAPI_CONNECTACTIVE_IND:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("PLCI             : 0x%04x\n",
			GET_WORD( a->connectactive_ind.plci));
		tn = (struct telno *)
			get_struct( a, &(a->connectactive_ind.telnolen));
		dump_c2telno_1byte( tn, msgend, "Connected address", 17);
		break;
	    case CAPI_CONNECTACTIVE_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->connectactive_resp.plci));
		break;
	    case CAPI_DISCONNECT_REQ:
		if ( ! check_len( msglen, 11, 11) ) break;
		printf("PLCI : 0x%04x\n", GET_WORD( a->disconnect_req.plci));
		printf("Cause: 0x%02x\n", a->disconnect_req.cause);
		break;
	    case CAPI_DISCONNECT_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->disconnect_conf.plci));
		dump_info( GET_WORD( a->disconnect_conf.info), 4);
		break;
	    case CAPI_DISCONNECT_IND:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->disconnect_ind.plci));
		dump_info( GET_WORD( a->disconnect_ind.info), 4);
		break;
	    case CAPI_DISCONNECT_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->disconnect_resp.plci));
		break;
	    case CAPI_LISTEN_REQ:
		if ( ! check_len( msglen, 17, 17) ) break;
		printf("Controller       : %d\n", a->listen_req.contrl);
		printf("Info mask        : 0x%08lx ",
			GET_DWORD( a->listen_req.info_mask));
			imask_out( GET_DWORD( a->listen_req.info_mask));
			printf("\n");
		printf("Serviced EAZ mask: 0x%04x ",
			GET_WORD( a->listen_req.eaz_mask));
			emask_out( GET_WORD( a->listen_req.eaz_mask));
			printf("\n");
		printf("Serviced SI mask : 0x%04x ",
			GET_WORD( a->listen_req.service_mask));
			smask_out( GET_WORD( a->listen_req.service_mask));
			printf("\n");
		break;
	    case CAPI_LISTEN_CONF:
		if ( ! check_len( msglen, 11, 11) ) break;
		printf("Controller: %d\n", a->listen_conf.contrl);
		dump_info( GET_WORD( a->listen_conf.info), 10);
		break;
	    case CAPI_GETPARAMS_REQ:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->getparams_req.plci));
		break;
	    case CAPI_GETPARAMS_CONF:
		if ( ! check_len( msglen, 19, -1) ) break;
		printf("PLCI             : 0x%04x\n",
			GET_WORD( a->getparams_conf.plci));
		printf("Controller       : %d\n", a->getparams_conf.contrl);
		printf("B-channel        : 0x%02x ", a->getparams_conf.channel);
			channel_out( a->getparams_conf.channel);
			printf("\n");
		dump_info( GET_WORD( a->getparams_conf.info), 17);
		printf("B3 link count    : %d\n", a->getparams_conf.linkcnt);
		printf("Service          : %d ", a->getparams_conf.service);
			si_out( a->getparams_conf.service);
			printf("\n");
		printf("Service add      : %d\n", a->getparams_conf.addinfo);
		printf("Serviced EAZ     : %c\n",
			isprint(a->getparams_conf.eaz) ?
			a->getparams_conf.eaz : '-');
		tn = (struct telno  *)
			get_struct( a, &(a->getparams_conf.telnolen));
		dump_c2telno_1byte( tn, msgend, "Connected address", 17);
		break;
	    case CAPI_INFO_REQ:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI     : 0x%04x\n", GET_WORD( a->info_req.plci));
		printf("Info mask: 0x%08lx ",
			GET_DWORD( a->info_req.info_mask));
			imask_out( GET_DWORD( a->info_req.info_mask));
			printf("\n");
		break;
	    case CAPI_INFO_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->info_conf.plci));
		dump_info( GET_WORD( a->info_conf.info), 4);
		break;
	    case CAPI_INFO_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("PLCI        : 0x%04x\n", GET_WORD( a->info_ind.plci));
		printf("Info number : 0x%04x ",
			GET_WORD( a->info_ind.info_number));
			switch ( GET_WORD( a->info_ind.info_number) ) {
			    case AI_CHARGE:  printf("(CHARGE)");  break;
			    case AI_DATE:    printf("(DATE)");    break;
			    case AI_CPS:     printf("(CPS)");     break;
			    case AI_DISPLAY: printf("(DISPLAY)"); break;
			    case AI_UUINFO:  printf("(UUINFO)");  break;
			    case AI_CAUSE:   printf("(CAUSE)");   break;
			    case AI_DAD:     printf("(DAD)");     break;
			    default:         printf("(???)");     break;
			};
			printf("\n");
		data = (struct userdata *) get_struct( a, &a->info_ind.infolen);
		dump_c2user_data( data, msgend, 0, "Info element", 12, 1, -1);
		break;
	    case CAPI_INFO_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->info_resp.plci));
		break;
	    case CAPI_DATA_REQ:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("PLCI     : 0x%04x\n", GET_WORD( a->data_req.plci));
		data = (struct userdata *)
			get_struct( a, &(a->data_req.structlen));
		dump_c2user_data( data, msgend, 0, "User data", 9, 1, -1);
		break;
	    case CAPI_DATA_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->data_conf.plci));
		dump_info( GET_WORD(a->data_conf.info), 4);
		break;
	    case CAPI_DATA_IND:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("PLCI     : 0x%04x\n", GET_WORD( a->data_ind.plci));
		data = (struct userdata *)
			get_struct( a, &(a->data_ind.structlen));
		dump_c2user_data( data, msgend, 0, "User data", 9, 1, -1);
		break;
	    case CAPI_DATA_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->data_resp.plci));
		break;
	    case CAPI_SELECTB2_REQ:
		if ( ! check_len( msglen, 12, -1) ) break;
		printf("PLCI               : 0x%04x\n",
			GET_WORD(a->selectb2_req.plci));
		printf("B2 protocol        : 0x%02x ", a->selectb2_req.proto);
			b2_out( a->selectb2_req.proto);
			printf("\n");
		if ( a->selectb2_req.proto == L2V110TRANS ||
		     a->selectb2_req.proto == L2V110SDLC  ||
		     a->selectb2_req.proto == L2V110X75   ||
		     a->selectb2_req.proto == L2V110SYNC  ||
		     a->selectb2_req.proto == L2MODEM        )
		{
		    dlpd_v110 = (struct dlpd_v110 *)
			get_struct( a, &(a->selectb2_req.dlpdlen));
		    if ( check_struct( (struct userdata *) dlpd_v110, msgend,
				   0, "DLPD", 19, 7, -1) )
		    {
			printf("    Data length    : %d\n",
				GET_WORD( dlpd_v110->data_length));
			printf("    Link address A : %d\n",
				dlpd_v110->link_addr_a);
			printf("    Link address B : %d\n",
				dlpd_v110->link_addr_b);
			printf("    Modulo mode    : %d\n",
				dlpd_v110->modulo_mode);
			printf("    Window size    : %d\n",
				dlpd_v110->window_size);
			printf("    V.110 User-Rate: 0x%02x\n",
				dlpd_v110->user_rate);
			if ( dlpd_v110->length >= sizeof( struct dlpd_v110) )
			{
			    data = get_struct( a, &(dlpd_v110->xid));
			    dump_c2user_data( data, msgend, 4,"XID",19, 1, -1);
			};
		    };
		} else {
		    dlpd = (struct dlpd *)
			get_struct( a, &(a->selectb2_req.dlpdlen));
		    if ( check_struct( (struct userdata *) dlpd, msgend,
				   0, "DLPD", 19, 6, -1) )
		    {
			printf("    Data length    : %d\n",
				GET_WORD( dlpd->data_length));
			printf("    Link address A : %d\n", dlpd->link_addr_a);
			printf("    Link address B : %d\n", dlpd->link_addr_b);
			printf("    Modulo mode    : %d\n", dlpd->modulo_mode);
			printf("    Window size    : %d\n", dlpd->window_size);
			if ( dlpd->length >= sizeof( struct dlpd) )
			{
			    data = get_struct( a, &(dlpd->xid));
			    dump_c2user_data( data, msgend, 4,"XID",19, 1, -1);
			};
		    };
		};
		break;
	    case CAPI_SELECTB2_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->selectb2_conf.plci));
		dump_info( GET_WORD( a->selectb2_conf.info), 4);
		break;
	    case CAPI_SELECTB3_REQ:
		if ( ! check_len( msglen, 12, -1) ) break;
		printf("PLCI                   : 0x%04x\n",
			GET_WORD( a->selectb3_req.plci));
		printf("B3 protocol            : 0x%02x ",
			a->selectb3_req.proto);
			b3_out(a->selectb3_req.proto);
			printf("\n");
		switch ( a->selectb3_req.proto )
		{
		case L3T90:
		case L3ISO8208:
		    ncpd_x25 = (struct ncpd_x25 *)
			get_struct( a, &(a->selectb3_req.ncpdlen));
		    if ( check_struct( (struct userdata *) ncpd_x25, msgend,
				0, "NCPD", 23, 13, 14) )
		    {
			printf("    LIC                : 0x%04x\n",
				GET_WORD( ncpd_x25->lic));
			printf("    HIC                : 0x%04x\n",
				GET_WORD( ncpd_x25->hic));
			printf("    LTC                : 0x%04x\n",
				GET_WORD( ncpd_x25->ltc));
			printf("    HTC                : 0x%04x\n",
				GET_WORD( ncpd_x25->htc));
			printf("    LOC                : 0x%04x\n",
				GET_WORD( ncpd_x25->loc));
			printf("    HOC                : 0x%04x\n",
				GET_WORD( ncpd_x25->hoc));
			printf("    Modulo mode        : 0x%02x\n",
				ncpd_x25->modulo_mode);
			if ( ncpd_x25->length == sizeof( ncpd_x25) )
			{
			    printf("    Default window size: 0x%02x\n",
				ncpd_x25->dflt_window_size);
			};
		    };
		    break;
		case L3T30:
		    ncpd_t30 = (struct ncpd_fax *)
			get_struct( a, &(a->selectb3_req.ncpdlen));
		    if ( check_struct( (struct userdata *) ncpd_t30, msgend,
				0, "NCPD", 23, 7, -1) )
		    {
			printf("    Resolution         : %d\n",
				ncpd_t30->resolution);
			printf("    Maximum xmit. rate : %d\n",
				ncpd_t30->max_speed);
			printf("    Data format        : %d\n",
				ncpd_t30->format);
			printf("    Transmission level : %d\n",
				ncpd_t30->xmit_level);
			station_id =
				get_struct( a, &(ncpd_t30->station_id_length));
			dump_c2user_data( station_id, msgend,
				4, "Station identification", 23, 1, -1);
			head_line = skip_struct( a, (char *) station_id);
			dump_c2user_data( head_line, msgend,
				4, "Headline", 23, 1, 32);
		    };
		    break;
		default:
		    ncpd = get_struct( a, &(a->selectb3_req.ncpdlen));
		    dump_c2user_data( ncpd, msgend, 0, "NCPD", 23, 1, -1);
		    break;
		};
		break;
	    case CAPI_SELECTB3_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->selectb3_conf.plci));
		dump_info( GET_WORD( a->selectb3_conf.info), 4);
		break;
	    case CAPI_LISTENB3_REQ:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->listenb3_req.plci));
		break;
	    case CAPI_LISTENB3_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->listenb3_conf.plci));
		dump_info( GET_WORD( a->listenb3_conf.info), 4);
		break;
	    case CAPI_CONNECTB3_REQ:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("PLCI    : 0x%04x\n", GET_WORD( a->connectb3_req.plci));
		data = get_struct( a, &(a->connectb3_req.ncpilen));
		dump_c2user_data( data, msgend, 0, "NCPI", 8, 1, -1);
		break;
	    case CAPI_CONNECTB3_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->connectb3_conf.plci));
		printf("NCCI: 0x%04x\n", GET_WORD( a->connectb3_conf.ncci));
		dump_info( GET_WORD( a->connectb3_conf.info), 4);
		break;
	    case CAPI_CONNECTB3_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI    : 0x%04x\n", GET_WORD( a->connectb3_ind.ncci));
		printf("PLCI    : 0x%04x\n", GET_WORD( a->connectb3_ind.plci));
		data = get_struct( a, &(a->connectb3_ind.ncpilen));
		dump_c2user_data( data, msgend, 0, "NCPI", 8, 1, -1);
		break;
	    case CAPI_CONNECTB3_RESP:
		if ( ! check_len( msglen, 12, -1) ) break;
		printf("NCCI    : 0x%04x\n", GET_WORD( a->connectb3_resp.ncci));
		printf("Reject  : %d\n", a->connectb3_resp.reject);
		data = get_struct( a, &(a->connectb3_resp.ncpilen));
		dump_c2user_data( data, msgend, 0, "NCPI", 8, 1, -1);
		break;
	    case CAPI_CONNECTB3ACTIVE_IND:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("NCCI    : 0x%04x\n",
			GET_WORD( a->connectb3active_ind.ncci));
		data = get_struct( a, &(a->connectb3active_ind.ncpilen));
		dump_c2user_data( data, msgend, 0, "NCPI", 8, 1, -1);
		break;
	    case CAPI_CONNECTB3ACTIVE_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("NCCI: 0x%04x\n",
			GET_WORD( a->connectb3active_resp.ncci));
		break;
	    case CAPI_DISCONNECTB3_REQ:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("NCCI    : 0x%04x\n",
			GET_WORD( a->disconnectb3_req.ncci));
		data = get_struct( a, &(a->disconnectb3_req.ncpilen));
		dump_c2user_data( data, msgend, 0, "NCPI", 8, 1, -1);
		break;
	    case CAPI_DISCONNECTB3_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->disconnectb3_conf.ncci));
		dump_info( GET_WORD( a->disconnectb3_conf.info), 4);
		break;
	    case CAPI_DISCONNECTB3_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI    : 0x%04x\n",
			GET_WORD( a->disconnectb3_ind.ncci));
		dump_info( GET_WORD( a->disconnectb3_ind.info), 8);
		data = get_struct( a, &(a->disconnectb3_ind.ncpilen));
		if ( check_struct( data, msgend, 0, "NCPI", 8, 1, -1) )
		{
		    ncpi_t30 = (struct ncpi_fax *) data;
		    if  ( ncpi_t30->length >= sizeof( struct ncpi_fax) - 1 &&
			  ncpi_t30->resolution <= 1 &&
			  ncpi_t30->speed <= 5 &&
			  ncpi_t30->format <= 7 ) /* assume T.30 protocol */
		    {
			printf("    Resolution            : %d\n",
				ncpi_t30->resolution);
			printf("    Last transmission rate: %d\n",
				ncpi_t30->speed);
			printf("    Data format           : %d\n",
				ncpi_t30->format);
			printf("    Number of pages       : %d\n",
				ncpi_t30->pages);
			received_id =
				get_struct( a, &(ncpi_t30->receiver_id_length));
			dump_c2user_data( received_id,
				(char *) ncpi_t30 + ncpi_t30->length,
				4, "Station identification", 26, 1, -1);
		        bad_pages = skip_struct( a, (char *) received_id);
			dump_c2user_data( bad_pages,
				(char *) ncpi_t30 + ncpi_t30->length,
				4, "Faulty pages", 26, 1, -1);
		    } else {
			capi_hexdump_d( data->data, data->length, 16, 0 + 4, 26);
		    };
		};
		break;
	    case CAPI_DISCONNECTB3_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->disconnectb3_resp.ncci));
		break;
	    case CAPI_GETB3PARAMS_REQ:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->getb3params_req.ncci));
		break;
	    case CAPI_GETB3PARAMS_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->getb3params_conf.ncci));
		printf("PLCI: 0x%04x\n", GET_WORD( a->getb3params_conf.plci));
		dump_info( GET_WORD( a->getb3params_conf.info), 4);
		break;
	    case CAPI_DATAB3_REQ:
		if ( ! check_len( msglen, 19, 19) ) break;
		printf("NCCI       : 0x%04x\n", GET_WORD( a->datab3_req.ncci));
		printf("Data length: %d\n", GET_WORD( a->datab3_req.datalen));
		printf("Data       : 0x%08lx\n", GET_DWORD(a->datab3_req.data));
		printf("Number     : 0x%02x\n", a->datab3_req.blknum);
		printf("Flags      : 0x%04x ", GET_WORD( a->datab3_req.flags));
			printf("%c ", GET_WORD( a->datab3_req.flags)
				& CAPI_QUALIFIER ? 'Q' : '-');
			printf("%c ", GET_WORD( a->datab3_req.flags)
				& CAPI_MORE_FLAG ? 'M' : '-');
			printf("%c ", GET_WORD( a->datab3_req.flags)
				& CAPI_DELIVERY  ? 'D' : '-');
			printf("\n");
		break;
	    case CAPI_DATAB3_CONF:
		if ( ! check_len( msglen, 13, 13) ) break;
		printf("NCCI  : 0x%04x\n", GET_WORD( a->datab3_conf.ncci));
		printf("Number: 0x%02x\n", a->datab3_conf.blknum);
		dump_info( GET_WORD( a->datab3_conf.info), 4);
		break;
	    case CAPI_DATAB3_IND:
		if ( ! check_len( msglen, 19, 19) ) break;
		printf("NCCI       : 0x%04x\n", GET_WORD( a->datab3_ind.ncci));
		printf("Data length: %d\n", GET_WORD( a->datab3_ind.datalen));
		printf("Data       : 0x%08lx\n", GET_DWORD(a->datab3_ind.data));
		printf("Number     : 0x%02x\n", a->datab3_ind.blknum);
		printf("Flags      : 0x%04x ", GET_WORD( a->datab3_ind.flags));
			printf("%c ", GET_WORD(a->datab3_ind.flags)
				& CAPI_QUALIFIER ? 'Q' : '-');
			printf("%c ", GET_WORD(a->datab3_ind.flags)
				& CAPI_MORE_FLAG ? 'M' : '-');
			printf("%c ", GET_WORD(a->datab3_ind.flags)
				& CAPI_DELIVERY  ? 'D' : '-');
			printf("\n");
		break;
	    case CAPI_DATAB3_RESP:
		if ( ! check_len( msglen, 11, 11) ) break;
		printf("NCCI  : 0x%04x\n", GET_WORD( a->datab3_resp.ncci));
		printf("Number: 0x%02x\n", a->datab3_resp.blknum);
		break;
	    case CAPI_RESETB3_REQ:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->resetb3_req.ncci));
		break;
	    case CAPI_RESETB3_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->resetb3_conf.ncci));
		dump_info( GET_WORD( a->resetb3_conf.info), 4);
		break;
	    case CAPI_RESETB3_IND:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->resetb3_ind.ncci));
		break;
	    case CAPI_RESETB3_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("NCCI: 0x%04x\n", GET_WORD( a->resetb3_resp.ncci));
		break;
	    case CAPI_HANDSET_IND:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI      : 0x%04x\n", GET_WORD( a->handset_ind.plci));
		printf("Controller: %d\n", a->handset_ind.contrl);
		printf("Status    : %c\n", a->handset_ind.state);
		break;
	    case CAPI_HANDSET_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->handset_resp.plci));
		break;
	    case CAPI_DTMF_REQ:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("PLCI         : 0x%04x\n", GET_WORD(a->dtmf_req.plci));
		printf("Tone duration: 0x%04x\n",
			GET_WORD(a->dtmf_req.tonedurat));
		printf("Gap duration : 0x%04x\n",
			GET_WORD(a->dtmf_req.gapdurat));
		data = get_struct( a, &(a->dtmf_req.dtmflen));
		dump_c2user_data( data, msgend, 0, "DTMF numbers", 13, 1, -1);
		break;
	    case CAPI_DTMF_CONF:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->dtmf_conf.plci));
		dump_info( GET_WORD( a->dtmf_conf.info), 4);
		break;
	    case CAPI_DTMF_IND:
		if ( ! check_len( msglen, 11, -1) ) break;
		printf("PLCI        : 0x%04x\n", GET_WORD( a->dtmf_ind.plci));
		data = get_struct( a, &(a->dtmf_ind.dtmflen));
		dump_c2user_data( data, msgend, 0, "DTMF numbers", 12, 1, -1);
		break;
	    case CAPI_DTMF_RESP:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("PLCI: 0x%04x\n", GET_WORD( a->dtmf_resp.plci));
		break;

/*************************************************************************/
/*	Remote-CAPI messages						 */
/*************************************************************************/

	    case CAPI_REGISTER_REQ:
		if ( ! check_len( msglen, 21, 21) ) break;
		printf("Buffer               : 0x%08lx\n",
			GET_DWORD( a->register_req.buffer));
		printf("messageBufferSize    : %d\n",
			GET_WORD( a->register_req.nmess));
		printf("maxLogicalConnections: %d\n",
			GET_WORD( a->register_req.nconn));
		printf("maxBDataBlocks       : %d\n",
			GET_WORD( a->register_req.ndblock));
		printf("maxBDataLen          : %d\n",
			GET_WORD( a->register_req.dblocksiz));
		printf("capiVersion          : %d\n",
			GET_BYTE( a->register_req.version));
		break;
	    case CAPI_REGISTER_CONF:
		if ( ! check_len( msglen, 10, 10) ) break;
		dump_c2info( GET_WORD( a->register_conf.info), 4);
		break;
	    case CAPI_GETMANUFACT_REQ:
		if ( ! check_len( msglen, 8, 8) ) break;
		break;
	    case CAPI_GETMANUFACT_CONF:
		if ( ! check_len( msglen, 9, -1) ) break;
		dump_c2user_data( (struct userdata *) 
			&(a->getmanufact_conf.structlen), msgend,
			0, "identification string", 21, 1, -1);
		break;
	    case CAPI_GETSERIAL_REQ:
		if ( ! check_len( msglen, 8, 8) ) break;
		break;
	    case CAPI_GETSERIAL_CONF:
		if ( ! check_len( msglen, 9, -1) ) break;
		dump_c2user_data( (struct userdata *) 
			&(a->getserial_conf.structlen), msgend,
			0, "serial number", 13, 1, -1);
		break;
	    case CAPI_GETVERSION_REQ:
		if ( ! check_len( msglen, 8, 8) ) break;
		break;
	    case CAPI_GETVERSION_CONF:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf(           "CAPI version        : %d\n",
			GET_WORD( a->getversion_conf.version));
		printf(           "Manufacturer version: %d\n",
			GET_WORD( (a->getversion_conf.version)+2));
		dump_c2user_data( (struct userdata *) 
			&(a->getversion_conf.structlen), msgend,
			0, "version information", 20, 1, -1);
		break;
	    case CAPI_GETPROFILE_REQ:
		if ( ! check_len( msglen, 10, 10) ) break;
		printf("CtrlNr: %d\n",
			GET_WORD( a->getprofile_req.ncontrl));
		break;
	    case CAPI_GETPROFILE_CONF:
		if ( ! check_len( msglen, 74, 74) ) break;
		dump_c2info( GET_WORD( a->getprofile_conf.info), 12);
		printf("CtrlNr      : 0x%04x\n",
			GET_WORD( a->getprofile_conf.ncontrl));
		printf("nChannel    : 0x%04x\n",
			GET_WORD( a->getprofile_conf.nchannel));
		printf("options     : 0x%08lx\n",
			GET_DWORD( a->getprofile_conf.options));
		printf("b1protocols : 0x%08lx\n",
			GET_DWORD( a->getprofile_conf.b1protocol));
		printf("b2protocols : 0x%08lx\n",
			GET_DWORD( a->getprofile_conf.b2protocol));
		printf("b3protocols : 0x%08lx\n",
			GET_DWORD( a->getprofile_conf.b3protocol));
		printf("reserved    : ");
			capi_hexdump_d( a->getprofile_conf.reserved, 24, 24, 0, 12);
		printf("manufacturer: ");
			capi_hexdump_d( a->getprofile_conf.manufacturer, 20,20, 0, 12);
		break;

	    case CAPI_CONTROL_REQ:
		if (! check_len( msglen, 13, -1) ) break;
		printf("Controller: %d\n", GET_WORD( a->control_req.contrl));
		printf("Type      : %d\n", GET_WORD( a->control_req.type));
		dump_c2user_data( (struct userdata *) 
			&(a->control_req.structlen), msgend,
			0, "Data", 10, 1, -1);
		break;
	    case CAPI_CONTROL_CONF:
		if (! check_len( msglen, 14, 14) ) break;
		printf("Controller: %d\n", GET_WORD( a->control_conf.contrl));
		printf("Type      : %d\n", GET_WORD( a->control_conf.type));
		dump_info( GET_WORD( a->control_conf.info), 10);
		break;
	    case CAPI_CONTROL_IND:
		if (! check_len( msglen, 13, -1) ) break;
		printf("Controller: %d\n", GET_WORD( a->control_ind.contrl));
		printf("Type      : %d\n", GET_WORD( a->control_ind.type));
		dump_c2user_data( (struct userdata *) 
			&(a->control_ind.structlen), msgend,
			0, "Data", 10, 1, -1);
		break;
	    case CAPI_CONTROL_RESP:
		if (! check_len( msglen, 13, -1) ) break;
		printf("Controller: %d\n", GET_WORD( a->control_resp.contrl));
		printf("Type      : %d\n", GET_WORD( a->control_resp.type));
		dump_c2user_data( (struct userdata *) 
			&(a->control_resp.structlen), msgend,
			0, "Data", 10, 1, -1);
		break;

/*************************************************************************/
/*	CAPI 2.0 messages						 */
/*************************************************************************/

	    case CAPI2_ALERT_REQ:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("PLCI                     : 0x%08lx\n",
			GET_DWORD( a->c2alert_req.plci));
		dump_c2additional_info( a,
			(struct addinfo *) &(a->c2alert_req.structlen),
			msgend, 25);
		break;
	    case CAPI2_ALERT_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2alert_conf.plci));
		dump_c2info( GET_WORD( a->c2alert_conf.info), 4);
		break;
	    case CAPI2_CONNECT_REQ:
		if ( ! check_len( msglen, 23, -1) ) break;
		printf("Controller               : %ld\n",
			GET_DWORD( a->c2connect_req.contrl));
		dump_c2cip_value( GET_WORD( a->c2connect_req.cip_value), 25);
		calledPartyNumberPtr = (struct telno *)
			get_struct( a, &(a->c2connect_req.structlen));
		dump_c2telno_1byte( calledPartyNumberPtr, msgend,
			"Called party number", 25);
		callingPartyNumberPtr = (struct telno2 *)
			skip_struct( a, (char *) calledPartyNumberPtr);
		dump_c2telno_2byte( callingPartyNumberPtr, msgend,
			"Calling party number", 25);
		calledPartySubaddressPtr = (struct telno *)
			skip_struct( a, (char *) callingPartyNumberPtr);
		dump_c2telno_1byte( calledPartySubaddressPtr, msgend,
			"Called party subaddress", 25);
		callingPartySubaddressPtr = (struct telno *)
			skip_struct( a, (char *) calledPartySubaddressPtr);
		dump_c2telno_1byte( callingPartySubaddressPtr, msgend,
			"Calling party subaddress", 25);
		bProtocolPtr = (struct bprotocol *)
			skip_struct( a, (char *) callingPartySubaddressPtr);
		dump_c2b_protocol( a, bProtocolPtr, 25);
		bcPtr = skip_struct( a, (char *) bProtocolPtr);
		dump_c2user_data( bcPtr, msgend,
			0, "BC", 25, 2, 11); /*Q.931: max length is 13 octets*/
		llcPtr = skip_struct( a, (char *) bcPtr);
		dump_c2user_data( llcPtr, msgend,
			0, "LLC", 25, 2, 14); /*Q.931: max length is 16 octets*/
		hlcPtr = skip_struct( a, (char *) llcPtr);
		dump_c2user_data( hlcPtr, msgend,
			0, "HLC", 25, 2, 3); /*Q.931: max length is 5 octets*/
		additionalInfoPtr = (struct addinfo *)
			skip_struct( a, (char *) hlcPtr);
		dump_c2additional_info( a, additionalInfoPtr, msgend, 25);
		break;
	    case CAPI2_CONNECT_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2connect_conf.plci));
		dump_c2info( GET_WORD( a->c2connect_conf.info), 4);
		break;
	    case CAPI2_CONNECT_IND:
		if ( ! check_len( msglen, 22, -1) ) break;
		printf("PLCI                     : 0x%08lx\n",
			GET_DWORD( a->c2connect_ind.plci));
		dump_c2cip_value( GET_WORD( a->c2connect_ind.cip_value), 25);
		calledPartyNumberPtr = (struct telno *)
			get_struct( a, &(a->c2connect_ind.structlen));
		dump_c2telno_1byte( calledPartyNumberPtr, msgend,
			"Called Party Number", 25);
		callingPartyNumberPtr = (struct telno2 *)
			skip_struct( a, (char *) calledPartyNumberPtr);
		dump_c2telno_2byte( callingPartyNumberPtr, msgend,
			"Calling Party Number", 25);
		calledPartySubaddressPtr = (struct telno *)
			skip_struct( a, (char *) callingPartyNumberPtr);
		dump_c2telno_1byte( calledPartySubaddressPtr, msgend,
			"Called Party Subaddress", 25);
		callingPartySubaddressPtr = (struct telno *)
			skip_struct( a, (char *) calledPartySubaddressPtr);
		dump_c2telno_1byte( callingPartySubaddressPtr, msgend,
			"Calling Party Subaddress", 25);
		bcPtr = skip_struct( a, (char *) callingPartySubaddressPtr);
		dump_c2user_data( bcPtr, msgend,
			0, "BC", 25, 2, 11); /*Q.931: max length is 13 octets*/
		llcPtr = skip_struct( a, (char *) bcPtr);
		dump_c2user_data( llcPtr, msgend,
			0, "LLC", 25, 2, 14); /*Q.931: max length is 16 octets*/
		hlcPtr = skip_struct( a, (char *) llcPtr);
		dump_c2user_data( hlcPtr, msgend,
			0, "HLC", 25, 2, 3); /*Q.931: max length is 5 octets*/
		additionalInfoPtr = (struct addinfo *)
			skip_struct( a, (char *) hlcPtr);
		dump_c2additional_info( a, additionalInfoPtr, msgend, 25);
		break;
	    case CAPI2_CONNECT_RESP:
		if ( ! check_len( msglen, 19, -1) ) break;
		printf("PLCI                     : 0x%08lx\n",
			GET_DWORD( a->c2connect_resp.plci));
		dump_c2reject( GET_WORD( a->c2connect_resp.reject), 25);
		bProtocolPtr = (struct bprotocol *)
			get_struct( a, &(a->c2connect_resp.structlen));
		dump_c2b_protocol( a, bProtocolPtr, 25);
		connectedNumberPtr = (struct telno2 *)
			skip_struct( a, (char *) bProtocolPtr);
		dump_c2telno_2byte( connectedNumberPtr, msgend,
			"Connected Number", 25);
		connectedSubaddressPtr = (struct telno *)
			skip_struct( a, (char *) connectedNumberPtr);
		dump_c2telno_1byte( connectedSubaddressPtr, msgend,
			"Connected Subaddress", 25);
		llcPtr = skip_struct( a, (char *) connectedSubaddressPtr);
		dump_c2user_data( llcPtr, msgend,
			0, "LLC", 25, 2, 14); /*Q.931: max length is 16 octets*/
		additionalInfoPtr = (struct addinfo *)
			skip_struct( a, (char *) llcPtr);
		dump_c2additional_info( a, additionalInfoPtr, msgend, 25);
		break;
	    case CAPI2_CONNECTACTIVE_IND:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("PLCI                : 0x%08lx\n",
			GET_DWORD( a->c2connectactive_ind.plci));
		connectedNumberPtr = (struct telno2 *)
			get_struct( a, &(a->c2connectactive_ind.structlen));
		dump_c2telno_2byte( connectedNumberPtr, msgend,
			"Connected Number", 20);
		connectedSubaddressPtr = (struct telno *)
			skip_struct( a, (char *) connectedNumberPtr);
		dump_c2telno_1byte( connectedSubaddressPtr, msgend,
			"Connected Subaddress", 20);
		llcPtr = (struct userdata *) skip_struct( a, 
			(char *) connectedSubaddressPtr);
		dump_c2user_data( llcPtr, msgend,
			0, "LLC", 20, 2, 14); /*Q.931: max length is 16 octets*/
		break;
	    case CAPI2_CONNECTACTIVE_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2connectactive_resp.plci));
		break;
	    case CAPI2_CONNECTB3ACTIVE_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2connectb3active_ind.ncci));
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2connectb3active_ind.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_CONNECTB3ACTIVE_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2connectb3active_resp.ncci));
		break;
	    case CAPI2_CONNECTB3_REQ:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("PLCI              : 0x%08lx\n",
			GET_DWORD( a->c2connectb3_req.plci));
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2connectb3_req.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_CONNECTB3_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2connectb3_conf.ncci));
		dump_c2info( GET_WORD( a->c2connectb3_conf.info), 4);
		break;
	    case CAPI2_CONNECTB3_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2connectb3_ind.ncci));
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2connectb3_ind.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_CONNECTB3_RESP:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2connectb3_resp.ncci));
		dump_c2reject( GET_WORD( a->c2connectb3_resp.reject), 18);
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2connectb3_resp.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_CONNECTB3T90ACTIVE_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2connectb3t90active_ind.ncci));
		ncpiPtr = (struct ncpi2_x25 *)	get_struct( a, 
			&(a->c2connectb3t90active_ind.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_CONNECTB3T90ACTIVE_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2connectb3t90active_resp.ncci));
		break;
	    case CAPI2_DATAB3_REQ:
		if ( ! check_len( msglen, 22, 22) ) break;
		printf("NCCI       : 0x%08lx\n",
			GET_DWORD( a->c2datab3_req.ncci));
		printf("Data       : 0x%08lx\n",
			GET_DWORD( a->c2datab3_req.dataptr));
		printf("Data Length: 0x%04x\n",
			GET_WORD( a->c2datab3_req.datalen));
		printf("Data Handle: 0x%04x\n",
			GET_WORD( a->c2datab3_req.handle));
		dump_c2flags( GET_WORD( a->c2datab3_req.flags));
		break;
	    case CAPI2_DATAB3_CONF:
		if ( ! check_len( msglen, 16, 16) ) break;
		printf("NCCI       : 0x%08lx\n",
			GET_DWORD( a->c2datab3_conf.ncci));
		printf("Data Handle: 0x%04x\n",
			GET_WORD( a->c2datab3_conf.handle));
		dump_c2info( GET_WORD( a->c2datab3_conf.info), 11);
		break;
	    case CAPI2_DATAB3_IND:
		if ( ! check_len( msglen, 22, 22) ) break;
		printf("NCCI       : 0x%08lx\n",
			GET_DWORD( a->c2datab3_ind.ncci));
		printf("Data       : 0x%08lx\n",
			GET_DWORD( a->c2datab3_ind.dataptr));
		printf("Data Length: 0x%04x\n",
			GET_WORD( a->c2datab3_ind.datalen));
		printf("Data Handle: 0x%04x\n",
			GET_WORD( a->c2datab3_ind.handle));
		dump_c2flags( GET_WORD( a->c2datab3_ind.flags));
		break;
	    case CAPI2_DATAB3_RESP:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("NCCI       : 0x%08lx\n",
			GET_DWORD( a->c2datab3_resp.ncci));
		printf("Data Handle: 0x%04x\n",
			GET_WORD( a->c2datab3_resp.handle));
		break;
	    case CAPI2_DISCONNECTB3_REQ:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2disconnectb3_req.ncci));
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2disconnectb3_req.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_DISCONNECTB3_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2disconnectb3_conf.ncci));
		dump_c2info( GET_WORD( a->c2disconnectb3_conf.info), 4);
		break;
	    case CAPI2_DISCONNECTB3_IND:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2disconnectb3_ind.ncci));
		dump_c2reasonB3( GET_WORD( a->c2disconnectb3_ind.reason_b3), 
				 18);
		/* TO DO: ncpi has different coding for T.30 !!! */
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2disconnectb3_ind.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_DISCONNECTB3_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2disconnectb3_ind.ncci));
		break;
	    case CAPI2_DISCONNECT_REQ:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("PLCI                     : 0x%08lx\n",
			GET_DWORD( a->c2disconnect_req.plci));
		additionalInfoPtr = (struct addinfo *)
			get_struct( a, &(a->c2disconnect_req.structlen));
		dump_c2additional_info( a, additionalInfoPtr, msgend, 25);
		break;
	    case CAPI2_DISCONNECT_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2disconnect_conf.plci));
		dump_c2info( GET_WORD( a->c2disconnect_conf.info), 4);
		break;
	    case CAPI2_DISCONNECT_IND:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI  : 0x%08lx\n",
			GET_DWORD( a->c2disconnect_ind.plci));
		dump_c2reason( GET_WORD( a->c2disconnect_ind.reason), 6);
		break;
	    case CAPI2_DISCONNECT_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2disconnect_resp.plci));
		break;
	    case CAPI2_FACILITY_REQ:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("Controller/PLCI/NCCI: 0x%08lx\n",
			GET_DWORD( a->c2facility_req.ident));
		printf("Facility Selector   : %d\n",
			GET_WORD( a->c2facility_req.selector));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2facility_req.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Parameter", 20, 1, -1);
		break;
	    case CAPI2_FACILITY_CONF:
		if ( ! check_len( msglen, 17, -1) ) break;
		printf("Controller/PLCI/NCCI: 0x%08lx\n",
			GET_DWORD( a->c2facility_conf.ident));
		dump_c2info( GET_WORD( a->c2facility_conf.info), 20);
		printf("Facility Selector   : %d\n",
			GET_WORD( a->c2facility_conf.selector));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2facility_conf.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Parameter", 20, 1, -1);
		break;
	    case CAPI2_FACILITY_IND:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("Controller/PLCI/NCCI: 0x%08lx\n",
			GET_DWORD( a->c2facility_ind.ident));
		printf("Facility Selector   : %d\n",
			GET_WORD( a->c2facility_ind.selector));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2facility_ind.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Parameter", 20, 1, -1);
		break;
	    case CAPI2_FACILITY_RESP:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("Controller/PLCI/NCCI: 0x%08lx\n",
			GET_DWORD( a->c2facility_resp.ident));
		printf("Facility Selector   : %d\n",
			GET_WORD( a->c2facility_resp.selector));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2facility_resp.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Parameter", 20, 1, -1);
		break;
	    case CAPI2_INFO_REQ:
		if ( ! check_len( msglen, 14, -1) ) break;
		printf("Controller/PLCI          : 0x%08lx\n",
			GET_DWORD( a->c2info_req.ident));
		calledPartyNumberPtr = (struct telno *)
			get_struct( a, &(a->c2info_req.structlen));
		dump_c2telno_1byte( calledPartyNumberPtr, msgend,
			"Called Party Number", 25);
		additionalInfoPtr = (struct addinfo *)
			skip_struct( a, (char *) calledPartyNumberPtr);
		dump_c2additional_info( a, additionalInfoPtr, msgend, 25);
		break;
	    case CAPI2_INFO_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2info_conf.ident));
		dump_c2info( GET_WORD( a->c2info_conf.info), 4);
		break;
	    case CAPI2_INFO_IND:
		if ( ! check_len( msglen, 15, -1) ) break;
		printf("Controller/PLCI: 0x%08lx\n",
			GET_DWORD( a->c2info_ind.ident));
		printf("Info Number    : 0x%04x\n",
			GET_WORD( a->c2info_ind.info));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2info_ind.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Info Element",15, 1, -1);
		break;
	    case CAPI2_INFO_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("Controller/PLCI: 0x%08lx\n",
			GET_DWORD( a->c2info_resp.ident));
		break;
	    case CAPI2_LISTEN_REQ:
		if ( ! check_len( msglen, 26, -1) ) break;
		printf("Controller              : %ld\n",
			GET_DWORD( a->c2listen_req.contrl));
		printf("Info Mask               : 0x%08lx\n",
			GET_DWORD( a->c2listen_req.info_mask));
		printf("CIP Mask                : 0x%08lx\n",
			GET_DWORD( a->c2listen_req.cip_mask));
		printf("CIP Mask 2              : 0x%08lx\n",
			GET_DWORD( a->c2listen_req.cip_mask2));
		callingPartyNumberPtr = (struct telno2 *)
			get_struct( a, &(a->c2listen_req.structlen));
		dump_c2telno_2byte( callingPartyNumberPtr, msgend,
			"Calling Party Number", 24);
		callingPartySubaddressPtr = (struct telno *)
			skip_struct( a, (char *) callingPartyNumberPtr);
		dump_c2telno_1byte( callingPartySubaddressPtr, msgend,
			"Calling Party Subaddress", 24);
		break;
	    case CAPI2_LISTEN_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("Controller: %ld\n",
			GET_DWORD( a->c2listen_conf.contrl));
		dump_c2info( GET_WORD( a->c2listen_conf.info), 10);
		break;
	    case CAPI2_MANUFACT_REQ:
		if ( ! check_len( msglen, 17, -1) ) break;
		printf("Controller           : %ld\n",
			GET_DWORD( a->c2manufact_req.contrl));
		printf("Manu ID              : 0x%08lx\n",
			GET_DWORD( a->c2manufact_req.manuid));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2manufact_req.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Manufacturer Specific", 21, 1, -1);
		break;
	    case CAPI2_MANUFACT_CONF:
		if ( ! check_len( msglen, 17, -1) ) break;
		printf("Controller           : %ld\n",
			GET_DWORD( a->c2manufact_conf.contrl));
		printf("Manu ID              : 0x%08lx\n",
			GET_DWORD( a->c2manufact_conf.manuid));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2manufact_conf.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Manu Specific", 21, 1, -1);
		break;
	    case CAPI2_MANUFACT_IND:
		if ( ! check_len( msglen, 17, -1) ) break;
		printf("Controller           : %ld\n",
			GET_DWORD( a->c2manufact_ind.contrl));
		printf("Manu ID              : 0x%08lx\n",
			GET_DWORD( a->c2manufact_ind.manuid));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2manufact_ind.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Manufacturer Specific", 21, 1, -1);
		break;
	    case CAPI2_MANUFACT_RESP:
		if ( ! check_len( msglen, 17, -1) ) break;
		printf("Controller           : %ld\n",
			GET_DWORD( a->c2manufact_resp.contrl));
		printf("Manu ID              : 0x%08lx\n",
			GET_DWORD( a->c2manufact_resp.manuid));
		dataPtr = (struct userdata *)
			get_struct( a, &(a->c2manufact_resp.structlen));
		dump_c2user_data( dataPtr, msgend,
			0, "Manufacturer Specific", 21, 1, -1);
		break;
	    case CAPI2_RESETB3_REQ:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2resetb3_req.ncci));
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2resetb3_req.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_RESETB3_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2resetb3_conf.ncci));
		dump_c2info( GET_WORD( a->c2resetb3_conf.info), 4);
		break;
	    case CAPI2_RESETB3_IND:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("NCCI              : 0x%08lx\n",
			GET_DWORD( a->c2resetb3_ind.ncci));
		ncpiPtr = (struct ncpi2_x25 *)
			get_struct( a, &(a->c2resetb3_ind.structlen));
		dump_c2ncpi_x25( ncpiPtr, msgend, 18);
		break;
	    case CAPI2_RESETB3_RESP:
		if ( ! check_len( msglen, 12, 12) ) break;
		printf("NCCI: 0x%08lx\n",
			GET_DWORD( a->c2resetb3_resp.ncci));
		break;
	    case CAPI2_SELECTB_REQ:
		if ( ! check_len( msglen, 13, -1) ) break;
		printf("PLCI            : 0x%08lx\n",
			GET_DWORD( a->c2selectb_req.plci));
		bProtocolPtr = (struct bprotocol *)
			get_struct( a, &(a->c2selectb_req.structlen));
		dump_c2b_protocol( a, bProtocolPtr, 16);
		break;
	    case CAPI2_SELECTB_CONF:
		if ( ! check_len( msglen, 14, 14) ) break;
		printf("PLCI: 0x%08lx\n",
			GET_DWORD( a->c2selectb_conf.plci));
		dump_c2info( GET_WORD( a->c2selectb_conf.info), 4);
		break;
	    default:
		break;
	}; /* end switch */
    }; /* end if long output */

    printf("\n");
    return 0;

dump_capimsg_error_dump:
	capi_hexdump_d( (char *) a, msglen, 0x10, 2, 6);
	return 1;
}

/***********************************************/
/*                                             */
/*   get error message text                    */
/*                                             */
/***********************************************/

CONST char *getErrorText( info)
unsigned long info;
{
    static struct {
	unsigned long	info;
	CONST char	*errorText;
    }		*msgPtr, capiError[] = {

	{ 0x0000, "(no error)"						},

	/*********************************************************
	 *	CAPI 1.1 error codes	
	 *********************************************************/

	{ 0x1001, "Application registration error"                        },
	{ 0x1002, "Wrong application id"                                  },
	{ 0x1003, "Message error"                                         },
	{ 0x1004, "Wrong capi command"                                    },
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
	{ 0x3106, "Dlpd incorrect"                                        },
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
	{ 0x3491, "requested facility not subscribed"                     },
	{ 0x34a0, "Outgoing calls barred"                                 },
	{ 0x34a1, "User Busy"                                             },
	{ 0x34a2, "negativer GBG-Vergleich"                               },
	{ 0x34a5, "als SPV nicht erlaubt"                                 },
	{ 0x34b0, "Reverse charging not allowed at origination"           },
	{ 0x34b1, "Reverse charging not allowed at destination"           },
	{ 0x34b2, "Reverse charging rejected"                             },
	{ 0x34b5, "Destination not obtainable"                            },
	{ 0x34b8, "Number changed"                                        },
	{ 0x34b9, "Out of order"                                          },
	{ 0x34ba, "User not responding"                                   },
	{ 0x34bb, "User access busy"                                      },
	{ 0x34bd, "Incomming calls barred"                                },
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
	{ 0x400a, "Line Disconnect during transfer"			  },
	{ 0x400b, "Disconnect before transer (remote does not support transfer mode)"},
	{ 0x400c, "Local Disconnect (SFF coding error)"			  },

	/*********************************************************
	 *	CAPI 2.0 Info values
	 *********************************************************/

/* CAPI 2.0 informativ values */

	{ 0x20000000, "request accepted / normal clearing, no cause available"},
	{ 0x20000001, "NCPI not supported by current protocol, NCPI ignored"},
	{ 0x20000002, "Flags not supported by current protocol, flags ignored"	},
	{ 0x20000003, "Alert already sent by another application"	},

/* CAPI 2.0 error information concerning CAPI_REGISTER */

	{ 0x20001001, "Too many applications"				},
	{ 0x20001002, "Logical block size too small, must be at least 128 bytes"	},
	{ 0x20001003, "Buffer exceeds 64 kByte"				},
	{ 0x20001004, "Message buffer size too small, must be at least 1024 bytes"	},
	{ 0x20001005, "Max. number of logical connections not supported"},
	{ 0x20001006, "Reserved"					},
	{ 0x20001007, "The message could not be accepted because of an internal busy condition"},
	{ 0x20001008, "OS Resource error (e.g. no memory)"		},
	{ 0x20001009, "COMMON-ISDN-API not installed"			},
	{ 0x2000100a, "Controller does not support external equipment"	},
	{ 0x2000100b, "Controller does only suport external equipment"	},

/* CAPI 2.0 error information concerning message exchange functions */

	{ 0x20001101, "Illegal application number"			},
	{ 0x20001102, "Illegal command or subcommand or message length less than 12 octets"},
	{ 0x20001103, "The message could not be accepted because of a queue full condition"},
	{ 0x20001104, "Queue is empty"					},
	{ 0x20001105, "Queue overflow, a message was lost"		},
	{ 0x20001106, "Unknown notification parameter"			},
	{ 0x20001107, "The message could not be accepted because of an internal busy condition" },
	{ 0x20001108, "OS resource error (e.g. no memory)"		},
	{ 0x20001109, "COMMON-ISDN-API not installed"			},
	{ 0x2000110a, "Controller does not support external equipment"	},
	{ 0x2000110b, "Controller does only suport external equipment"	},

/* CAPI 2.0 error information concerning resource/coding problems */

	{ 0x20002001, "Message not supported in current state"		},
	{ 0x20002002, "Illegal Controller/PLCI/NCCI"			},
	{ 0x20002003, "Out of PLCI"					},
	{ 0x20002004, "Out of NCCI"					},
	{ 0x20002005, "Out of LISTEN"					},
	{ 0x20002006, "Out of FAX resources (protocol T.30)"		},
	{ 0x20002007, "Illegal Message parameter coding"		},

/* CAPI 2.0 error information concerning requested services */

	{ 0x20003001, "B1 protocol not supported"			},
	{ 0x20003002, "B2 protocol not supported"			},
	{ 0x20003003, "B3 protocol not supported"			},
	{ 0x20003004, "B1 protocol parameter not supported"		},
	{ 0x20003005, "B2 protocol parameter not supported"		},
	{ 0x20003006, "B3 protocol parameter not supported"		},
	{ 0x20003007, "B protocol combination not supported"		},
	{ 0x20003008, "NCPI not supported"				},
	{ 0x20003009, "CIP Value unknown"				},
	{ 0x2000300a, "Flags not supported (reserved bits)"		},
	{ 0x2000300b, "Facility not supported"				},
	{ 0x2000300c, "Data length not supported by currect protocol"	},
	{ 0x2000300d, "Reset procedure not supported by current protocol"},

	/*********************************************************
	 *	CAPI 2.0 Reason and Reason_B3 values	
	 *********************************************************/

/* CAPI 2.0 protocol independent reason */

	{ 0x20003301, "Protocol error layer 1 (broken line or B-channel removed by signalling protocol)"},
	{ 0x20003302, "Protocol error layer 2"				},
	{ 0x20003303, "Protocol error layer 3"				},
	{ 0x20003304, "Another application got that call"		},
	
/* CAPI 2.0 T.30 specific reason */

	{ 0x20003311, "connecting not successful (remote station is no fax G3 machine)"},
	{ 0x20003312, "connecting not successful (training error)"	},
	{ 0x20003313, "disconnected before transfer (remote station does not support transfer mode, e.g. resolution)"},
	{ 0x20003314, "disconnected during transfer (remote abort)"	},
	{ 0x20003315, "disconnected during transfer (remote procedure error, e.g. unsuccessful repetition of T.30 commands)"},
	{ 0x20003316, "disconnected during transfer (local tx data underrun)"},
	{ 0x20003317, "disconnected during transfer (local rx data overflow)"},
	{ 0x20003318, "disconnected during transfer (local abort)"	},
	{ 0x20003319, "illegal parameter coding (e.g. SFF coding error)"},

	/*********************************************************************
	 * CAPI 2.0 Reason values as of ETS 300 102-1/Q.931 chapter 4.5.12 
	 * ('xx' in '34xx' is octet 4 of Cause information element)       
	 *********************************************************************/

	{ 0x20003400, "Normal call clearing, no cause available"	},
        { 0x20003481, "Unallocated (unassigned) number"			},
        { 0x20003482, "No route to specified transit network"		},
        { 0x20003483, "No route to destination"				},
        { 0x20003486, "Channel unacceptable"				},
        { 0x20003487, "Call awarded and being delivered in an established channel"	},
        { 0x20003490, "Normal call clearing"				},
        { 0x20003491, "User busy"					},
        { 0x20003492, "No user responding"				},
        { 0x20003493, "No answer from user (user alerted)"		},
        { 0x20003495, "Call rejected"					},
        { 0x20003496, "Number changed"					},
        { 0x2000349a, "Non-selected user clearing"			},
        { 0x2000349b, "Destination out of order"			},
        { 0x2000349c, "Invalid number format"				},
        { 0x2000349d, "Facility rejected"				},
        { 0x2000349e, "Response to STATUS ENQUIRY"			},
        { 0x2000349f, "Normal, unspecified"				},
        { 0x200034a2, "No circuit/channel available"			},
        { 0x200034a6, "Network out of order"				},
        { 0x200034a9, "Temporary failure"				},
        { 0x200034aa, "Switching equipment congestion"			},
        { 0x200034ab, "Access information discarded"			},
        { 0x200034ac, "Requested circuit/channel not available"		},
        { 0x200034af, "Resources unavailable, unspecified"		},
	{ 0x200034b1, "Quality of service unavailable"			},
        { 0x200034b2, "Requested facility not subscribed"		},
        { 0x200034b9, "Bearer capability not authorized"		},
        { 0x200034ba, "Bearer capability not presently available"	},
        { 0x200034bf, "Service or option not available, unspecified"	},
        { 0x200034c1, "Bearer capability not implemented"		},
        { 0x200034c2, "Channel type not implemented"			},
        { 0x200034c5, "Requested facility not implemented"		},
        { 0x200034c6, "Only restricted digital information bearer capability is available"},
        { 0x200034cf, "Service or option not implemented, unspecified"	},
        { 0x200034d1, "Invalid call reference value"			},
        { 0x200034d2, "Identified channel does not exist"		},
        { 0x200034d3, "A suspended call exists, but this call identity does not"	},
        { 0x200034d4, "Call identity in use"				},
        { 0x200034d5, "No call suspended"				},
        { 0x200034d6, "Call having the requested call identity has been cleared"	},
        { 0x200034d8, "Incompatible destination"			},
        { 0x200034db, "Invalid transit network selection"		},
        { 0x200034df, "Invalid message, unspecified"			},
        { 0x200034e0, "Mandatory information element is missing"	},
        { 0x200034e1, "Message type non-existent or not implemented"	},
        { 0x200034e2, "Message not compatible with call state or message type non-existent or not implemented"},
        { 0x200034e3, "Information element non-existent or not implemented"},
        { 0x200034e4, "Invalid information element contents"		},
        { 0x200034e5, "Message not compatible with call state"		},
        { 0x200034e6, "Recovery on timer expiry"			},
        { 0x200034ef, "Protocol error, unspecified"			},
        { 0x200034ff, "Interworking, unspecified"			},

/* end of list mark for info number search */

	{ 0xffffffff, "??? (undecoded error code)"			},
    };

    for (msgPtr = capiError;
	 msgPtr->info != info && msgPtr->info != 0xffffffff;
	 msgPtr++);

    return msgPtr->errorText;
}

/**********************************************/
/*                                            */
/*   dump info (reason) value and error text  */
/*                                            */
/**********************************************/

void dump_c2info( info, descLen)
unsigned short	info;
int		descLen;
{
    printf("%-*s: 0x%04x %s\n", descLen, "Info", info,
		getErrorText( 0x20000000 | (unsigned long) info));
}

void dump_c2reason( info, descLen)
unsigned short	info;
int		descLen;
{
    printf("%-*s: 0x%04x %s\n", descLen, "Reason", info,
		getErrorText( 0x20000000 | (unsigned long) info));
}

void dump_c2reasonB3( info, descLen)
unsigned short	info;
int		descLen;
{
    printf("%-*s: 0x%04x %s\n", descLen, "ReasonB3", info,
		getErrorText( 0x20000000 | (unsigned long) info));
}

void dump_info( info, descLen)
unsigned short	info;
int		descLen;
{
    printf("%-*s: 0x%04x %s\n", descLen, "Info", info,
		getErrorText( (unsigned long) info));
}

/************************************************************************/
/*									*/
/*	capi_msgOut							*/
/*									*/
/************************************************************************/
CONST char *capi_msgOut( a)
union CAPI_primitives *a;
{
    static struct {
	long		PRIM_type;
	CONST char	*text;
    } *msgp, apimsg[] = {
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
        { CAPI2_CONNECTACTIVE_IND,       "CAPI2_CONNECTACTIVE_IND"        },
        { CAPI2_CONNECTACTIVE_RESP,      "CAPI2_CONNECTACTIVE_RESP"       },
        { CAPI2_INFO_REQ ,               "CAPI2_INFO_REQ"                 },
        { CAPI2_INFO_CONF,               "CAPI2_INFO_CONF"                },
        { CAPI2_INFO_IND,                "CAPI2_INFO_IND"                 },
        { CAPI2_INFO_RESP,               "CAPI2_INFO_RESP"                },
        { CAPI2_CONNECTB3_REQ,           "CAPI2_CONNECTB3_REQ"            },
        { CAPI2_CONNECTB3_CONF,          "CAPI2_CONNECTB3_CONF"           },
        { CAPI2_CONNECTB3_IND,           "CAPI2_CONNECTB3_IND"            },
        { CAPI2_CONNECTB3_RESP,          "CAPI2_CONNECTB3_RESP"           },
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

        { CAPI_REGISTER_REQ,             "CAPI_REGISTER_REQ"              },
        { CAPI_REGISTER_CONF,            "CAPI_REGISTER_CONF"             },
        { CAPI_RELEASE_REQ,              "CAPI_RELEASE_REQ"               },
        { CAPI_RELEASE_CONF,             "CAPI_RELEASE_CONF"              },
        { CAPI_GETMANUFACT_REQ,          "CAPI_GETMANUFACT_REQ"           },
        { CAPI_GETMANUFACT_CONF,         "CAPI_GETMANUFACT_CONF"          },
        { CAPI_GETVERSION_REQ,           "CAPI_GETVERSION_REQ"            },
        { CAPI_GETVERSION_CONF,          "CAPI_GETVERSION_CONF"           },
        { CAPI_GETSERIAL_REQ,            "CAPI_GETSERIAL_REQ"             },
        { CAPI_GETSERIAL_CONF,           "CAPI_GETSERIAL_CONF"            },
        { CAPI_GETPROFILE_REQ,           "CAPI_GETPROFILE_REQ"            },
        { CAPI_GETPROFILE_CONF,          "CAPI_GETPROFILE_CONF"           },
	{ CAPI_CONTROL_REQ,		 "CAPI_CONTROL_REQ"		  },
	{ CAPI_CONTROL_CONF,		 "CAPI_CONTROL_CONF"		  },
	{ CAPI_CONTROL_IND,		 "CAPI_CONTROL_IND"		  },
	{ CAPI_CONTROL_RESP,		 "CAPI_CONTROL_RESP"		  },

	{ -1,				 "UNKNOWN CAPI MESSAGE!"	  }
    };

    for (msgp = apimsg;
	 msgp->PRIM_type != -1 && msgp->PRIM_type != GET_PRIMTYPE( a);
	 msgp++);

    return msgp->text;
}

/************************************************************************/
/*									*/
/*	emask_out							*/
/*									*/
/************************************************************************/
int emask_out( eazmask)
unsigned long eazmask;
{
    unsigned long i;

    for (i=0; i<10; i++) {
	if (eazmask & (1<<i)) {
	    printf( "%ld ", i);
	}
    }
    return 0;
}

/************************************************************************/
/*									*/
/*	smask_out							*/
/*									*/
/************************************************************************/
int smask_out( simask)
unsigned short simask;
{
    unsigned long i;

    for (i=0; i<16; i++) {
	if (simask & (1<<i)) {
	    printf( "%ld ", i ? i : 16);
	}
    }
    return 0;
}

/************************************************************************/
/*									*/
/*	imask_out							*/
/*									*/
/************************************************************************/
int imask_out( infomask )
unsigned long infomask;
{
    if (infomask & CAPI_ICHARGE) 	printf("CHARGE ");
    if (infomask & CAPI_IDATE) 		printf("DATE ");
    if (infomask & CAPI_IDISPLAY) 	printf("DISP ");
    if (infomask & CAPI_IUUINFO) 	printf("UUI ");
    if (infomask & CAPI_ICAUSE) 	printf("CAUSE ");
    if (infomask & CAPI_ISTATE) 	printf("STATE ");
    if (infomask & CAPI_IDESTINATION) 	printf("DAD ");
    if (infomask & CAPI_IDTMF) 		printf("DTMF ");
    if (infomask & CAPI_ISPV) 		printf("SPV ");
    if (infomask & CAPI_ISUBADDR) 	printf("SUB ");

    return 0;
}

/************************************************************************/
/*									*/
/*	b2_out								*/
/*									*/
/************************************************************************/
int b2_out( b2 )
unsigned char b2;
{
    switch (b2) {
	case L2X75:   	  printf("(X.75)");    	   break;
	case L2HDLCCRC:   printf("(HDLC)");    	   break;
	case L2TRANS:     printf("(TRANS)");   	   break;
	case L2SDLC:      printf("(SDLC)");    	   break;
	case L2X75BTX:    printf("(X.75 BTX)");    break;
	case L2FAX:       printf("(FAX G3)");      break;
	case L2LAPD:  	  printf("(LAPD)");    	   break;
	case L2V110TRANS: printf("(V.110 TRANS)"); break;
	case L2V110SDLC:  printf("(V.110 SDLC)");  break;
	case L2V110X75:	  printf("(V.110 X.75)");  break;
	case L2TXONLY:	  printf("(TX ONLY)");     break;
	case L2MODEM:	  printf("(MODEM)");       break;
	default:	  printf("(???)");	   break;
    }
    return 0;
}

/************************************************************************/
/*									*/
/*	b3_out								*/
/*									*/
/************************************************************************/
int b3_out( b3 )
unsigned char b3;
{
    switch (b3) {
	case L3T70NL:	printf("(T.70 NL)");     break;
	case L3ISO8208:	printf("(ISO 8208)");    break;
	case L3T90:    	printf("(T.90)");        break;
	case L3TRANS:	printf("(TRANS)");	 break;
	case L3T30:	printf("(T.30)");	 break;
	case L3PPP:	printf("(PPP)");	 break;
	case L3PPPASYNC:printf("(PPPasync)");	 break;
	default:	printf("(???)");	 break;
    }
    return 0;
}

/************************************************************************/
/*									*/
/*	si_out								*/
/*									*/
/************************************************************************/
int si_out( service )
unsigned char service;
{
    switch (service) {
	case SI_PHONE:		printf("(Phone)");	    break;
	case SI_ABSERVICES:    	printf("(a/b)");	    break;
	case SI_X21:		printf("(X21)");	    break;
	case SI_FAXG4:	    	printf("(Fax-G4)");	    break;
	case SI_BTX:	    	printf("(BTX)");	    break;
	case SI_DATA:	    	printf("(Data)");	    break;
	case SI_X25:	    	printf("(X.25)");	    break;
	case SI_TELETEX:	printf("(Teletex)");	    break;
	case SI_MIXEDMODE:	printf("(Mixedmode)");	    break;
	case SI_REMOTECTRL:     printf("(RemoteCtrl)");	    break;
	case SI_GRAPHTEL:       printf("(Graphtel)");	    break;
	case SI_VIDEOTEXT:      printf("(Videotext)");	    break;
	case SI_VIDEOPHONE:     printf("(Videophone)");	    break;
	default:	    	printf("(???)");	    break;
    }
    return 0;
}

/************************************************************************/
/*									*/
/*	channel_out							*/
/*									*/
/************************************************************************/
int channel_out( channel )
unsigned char channel;
{
    printf("(");
    switch (channel & 0x07) {
	case 0x00: printf("no channel, ");	break;
	case 0x01: printf("B1-channel, ");	break;
	case 0x02: printf("B2-channel, ");	break;
	case 0x03: printf("any channel, ");	break;
	case 0x04: printf("D-channel, ");	break;
	default:   printf("????, ");		break;
    }
    if (channel & 0x08) {
	printf("exclusiv");
    } else {
	printf("preferred");
    }
    printf(")");
    return 0;
}

/************************************************************************/
/*									*/
/*	utilities for scanning capi structs				*/
/*									*/
/************************************************************************/

struct userdata *get_struct( msg, ptr)
union CAPI_primitives	*msg;
char			*ptr;
{
    char *end;

    end = (char *) msg + GET_LEN( msg);
 
    if ( ptr >= end )
    {
	/* should not happen, if calling function is coded correctly */
	printf("ERROR IN get_struct()!\n");
	return NULL;
    };
    return (struct userdata *) ptr;
}

struct userdata *skip_struct( msg, ptr)
union CAPI_primitives	*msg;
char			*ptr;
{
    char		*end;
    struct userdata	*data;

    data = (struct userdata *) ptr;
    end  = (char *) msg + GET_LEN( msg);
 
    data = (struct userdata *) &(data->data[data->length]);
    if ( (char *) data >= end )
    {
	printf("STRUCT FOLLOWING EXCEEDS MESSAGE LENGTH!\n");
	return NULL;
    };
    return data;
}

int check_struct( structPtr, structEnd, indent, description, descLen, min, max)
struct userdata *structPtr;
char 		*structEnd;
int		indent;
CONST char	*description;
int		descLen;
int		min;
int		max;
{
    /* if get/skip_struct failed, ignore struct */
    if ( ! structPtr )
	return 0;

    /* print struct's description and length */
    printf("%*s%-*s: [%04d]", indent, "", descLen - indent, description, 
			       structPtr->length);

    /* test, if struct contains data at all */
    if ( structPtr->length == 0 )
    {
	printf(" (empty)\n");
	return 0;
    };
    
    /* test, if min <= struct length <= max */
    if ( min != -1 && structPtr->length < min )
    {
	printf(" STRUCT LENGTH TOO SMALL (minimum is [%04d])!\n", min);
	return 0;
    };
    if ( max != -1 && structPtr->length > max )
    {
	printf(" STRUCT LENGTH TOO BIG (maximum is [%04d])!\n", max);
	return 0;
    };

    /* test, if struct length exceeds the embedding message/struct */
    if ( (char *) structPtr + structPtr->length > structEnd )
    {
	if ( indent == 0 )
	    printf(" STRUCT LENGTH EXCEEDS CAPI MESSAGE LENGTH!\n");
	else
	    printf(" STRUCT LENGTH EXCEEDS EMBEDDING STRUCT LENGTH!\n");
	return 0;
    };

    printf("\n");
    return 1;		/* ready to print struct's data area */
}

int check_len( len, min, max)
int len;
int min;
int max;
{
    if ( min != -1 && len < min )
    {
	printf("MESSAGE LENGTH TOO SMALL (minimum is [%04d])!\n", min);
        return 0;
    };

    if ( max != -1 && len > max )
    {
	printf("MESSAGE LENGTH TOO BIG (maximum is [%04d])!\n", max);
        return 0;
    };

    return 1;	/* message length inbetween valid limits */
}

/************************************************************************/
/*									*/
/*	dump routines for various CAPI-2.0 structures			*/
/*									*/
/************************************************************************/

void dump_c2additional_info( msgPtr, addInfoPtr, msgend, descLen)
union CAPI_primitives	*msgPtr;
struct addinfo		*addInfoPtr;
char			*msgend;
int			descLen;
{
    struct bchaninfo	*bChannelInformationPtr;
    struct userdata	*keypadFacilityPtr, *userUserDataPtr,
			*facilityDataArrayPtr;

    if ( check_struct( (struct userdata *) addInfoPtr, msgend,
			0, "Additional Info", descLen, 2, -1) )
    {
	msgend = (char *) addInfoPtr + addInfoPtr->length;

	/* B Channel Information (struct!) */
	bChannelInformationPtr = (struct bchaninfo *)
		get_struct( msgPtr, &(addInfoPtr->structlen));
	if ( check_struct( (struct userdata *) bChannelInformationPtr, msgend,
				4, "B Channel Information", descLen, 2, -1) )
	{
	    printf("        %-*s: %d\n", descLen - 8, "Channel",
			GET_WORD( bChannelInformationPtr->channel));
            if (GET_WORD(bChannelInformationPtr->channel) == 4)
                dump_c2user_data(&bChannelInformationPtr->chid, msgend, 
                          8, "Channel Identification", descLen, 1, -1);
	};

	/* Keypad facility (struct) -> ETS 300 102-1 / Q.931 */
	keypadFacilityPtr = (struct userdata *)
		skip_struct( msgPtr, (char *) bChannelInformationPtr);
	dump_c2user_data( keypadFacilityPtr, msgend,
			  4, "Keypad Facility", descLen, 3, 34);

	/* User-user data (struct) -> ETS 300 102-1 / Q.931 */
	userUserDataPtr = (struct userdata *)
		skip_struct( msgPtr, (char *) keypadFacilityPtr);
	dump_c2user_data( userUserDataPtr, msgend,
			  4, "User-User Data", descLen, 4, -1);

	/* Facility data array (struct) -> ETS 300 102-1 / Q.931 */
	facilityDataArrayPtr = (struct userdata *)
		skip_struct( msgPtr, (char *) userUserDataPtr);
	dump_c2user_data( facilityDataArrayPtr, msgend,
			  4, "Facility", descLen, 6, -1);
    };
}

void dump_c2cip_value( cip_value, descLen)
unsigned short	cip_value;
int		descLen;
{
    printf("%-*s: %d (", descLen, "CIP Value", cip_value);
    switch ( cip_value )
    {
    case 0: printf("- (no predefined profile)"); break;
    case 1: printf("Speech"); break;
    case 2: printf("unrestricted digital information"); break;
    case 3: printf("restricted digital information"); break;
    case 4: printf("3.1 kHz audio"); break;
    case 5: printf("7 kHz audio"); break;
    case 6: printf("video"); break;
    case 7: printf("packet mode"); break;
    case 8: printf("56 kBit/s rate adaption"); break;
    case 9: printf("unrestricted digital information with tones/announcments"); break;
    case 16: printf("Telephony"); break;
    case 17: printf("Facsimile Group 2/3"); break;
    case 18: printf("Facsimile Group 4 Class 1"); break;
    case 19: printf("Teletex service basic and mixed mode and facsimile service Group 4 Classes II and III"); break;
    case 20: printf("Teletex service basic and processable mode"); break;
    case 21: printf("Teletex service basic mode"); break;
    case 22: printf("International inter working for Videotex"); break;
    case 23: printf("Telex"); break;
    case 24: printf("Message Handling Systems according to X.400"); break;
    case 25: printf("OSI application according to X.200"); break;
    case 26: printf("7 kHz Telephony"); break;
    case 27: printf("Video Telephony first connection"); break;
    case 28: printf("Video Telephony second connection"); break;
    default: printf("??? (unknown CIP value)"); break;
    };
    printf(")\n");
}

void dump_c2telno_1byte( numberPtr, msgend, description, descLen)
struct telno	*numberPtr;
char		*msgend;
CONST char	*description;
int		descLen;
{
    int i;

    if ( check_struct( (struct userdata *) numberPtr, msgend,
		0, description, descLen, 2, 23 - 2) )
    {
	printf("    %-*s: 0x%02x\n", descLen - 4, "Type", numberPtr->type);
	printf("    %-*s: ", descLen - 4, "Digits"); 
	for (i = 0; i < numberPtr->length - 1; i++)
	    printf("%c", numberPtr->no[i]);
	printf("\n");
    };
}

void dump_c2telno_2byte( numberPtr, msgend, description, descLen)
struct telno2	*numberPtr;
char		*msgend;
CONST char	*description;
int		descLen;
{
    int i;

    if ( check_struct( (struct userdata *) numberPtr, msgend,
		0, description, descLen, 3, 24 - 2) )
    {
	printf("    %-*s: 0x%02x\n", descLen - 4, "Type", numberPtr->type);
	printf("    %-*s: 0x%02x\n", descLen - 4, "Indicator", 
		numberPtr->indicator);
	printf("    %-*s: ", descLen - 4, "Digits");
	for (i = 0; i < numberPtr->length - 2; i++)
	    printf("%c", numberPtr->no[i]);
	printf("\n");
    };
}

void dump_c2user_data( ptr, msgend, indent, description, descLen, min, max)
struct userdata	*ptr;
char		*msgend;
int		indent;
CONST char	*description;
int		descLen;
int		min;
int		max;
{
    if ( check_struct( ptr, msgend, indent, description, descLen, min, max) )
    {
	capi_hexdump_d( ptr->data, ptr->length, 16, indent + 4, descLen);
    };
}

void dump_c2b_protocol( msg, protocolPtr, descLen)
union CAPI_primitives	*msg;
struct bprotocol	*protocolPtr;
int			descLen;
{
    struct b1config		*b1ConfigurationPtr;
    struct b2config		*b2ConfigurationPtr;
    struct b3config		*b3ConfigurationPtr;
    struct b3config_faxg3	*b3Configuration_faxg3Ptr;
    struct userdata		*dataPtr;
    char			*msgend;

    msgend = (char *) msg + GET_LEN( msg) - 1;

    if ( check_struct( (struct userdata *) protocolPtr, msgend,
		0, "B Protocol", descLen, 9, -1) )
    {
	msgend = (char *) protocolPtr + protocolPtr->length;

	/* B protocol words */
	dump_c2b_name( 1, GET_WORD( protocolPtr->b1proto), descLen);
	dump_c2b_name( 2, GET_WORD( protocolPtr->b2proto), descLen);
	dump_c2b_name( 3, GET_WORD( protocolPtr->b3proto), descLen);

	/* B1 configuration struct */
	b1ConfigurationPtr = (struct b1config *)
		get_struct( msg, &(protocolPtr->structlen));
	if ( check_struct( (struct userdata *) b1ConfigurationPtr, msgend,
			   4, "B1 Configuration", descLen, 8, 12) )
	{
	    printf("        %-*s: %d\n", descLen - 8, "Rate",
		    GET_WORD( b1ConfigurationPtr->rate));
	    printf("        %-*s: %d\n", descLen - 8, "Bits per char",
		    GET_WORD( b1ConfigurationPtr->bpc));
	    printf("        %-*s: %d\n", descLen - 8, "Parity",
		    GET_WORD( b1ConfigurationPtr->parity));
	    printf("        %-*s: %d\n", descLen - 8, "Stop bits",
		    GET_WORD( b1ConfigurationPtr->stopbits));
	};

	/* B2 configuration struct */
	b2ConfigurationPtr = (struct b2config *)
		skip_struct( msg, (char *) b1ConfigurationPtr);
	if ( check_struct( (struct userdata *) b2ConfigurationPtr, msgend,
			   4, "B2 Configuration", descLen, 2, -1) )
	{
	    printf("        %-*s: %d\n", descLen - 8, "Address A",
		    GET_BYTE( b2ConfigurationPtr->addressA));
	    printf("        %-*s: %d\n", descLen - 8, "Address B",
		    GET_BYTE( b2ConfigurationPtr->addressB));
	    printf("        %-*s: %d\n", descLen - 8, "Module Mode",
		    GET_BYTE( b2ConfigurationPtr->moduloMode));
	    printf("        %-*s: %d\n", descLen - 8, "Window Size",
		    GET_BYTE( b2ConfigurationPtr->windowSize));
	    dataPtr = (struct userdata *) &(b2ConfigurationPtr->xidlen);
	    dump_c2user_data( dataPtr, (char *) b2ConfigurationPtr +
		b2ConfigurationPtr->length, 8, "XID", descLen, 1, -1);
	};

	/* B3 configuration struct */
	switch ( GET_WORD( protocolPtr->b3proto) )
	{
	case B3T30:
	case B3T30EXT:
	    b3Configuration_faxg3Ptr = (struct b3config_faxg3 *)
		    skip_struct( msg, (char *) b2ConfigurationPtr);
	    if ( check_struct( (struct userdata *)
			b3Configuration_faxg3Ptr, msgend,
			4, "B3 Configuration", descLen, 6, -1) )
	    {
		printf("        %-*s: %d\n", descLen - 8, "Resolution",
			GET_WORD( b3Configuration_faxg3Ptr->resolution));
		printf("        %-*s: %d\n", descLen - 8, "Format",
			GET_WORD( b3Configuration_faxg3Ptr->format));
		dataPtr = (struct userdata *)
		    get_struct( msg, &(b3Configuration_faxg3Ptr->structlen));
		dump_c2user_data( dataPtr, (char *) b3Configuration_faxg3Ptr +
			b3Configuration_faxg3Ptr->length,
			8, "Station ID", descLen, 1, -1);
		dataPtr = (struct userdata *)
			skip_struct( msg, (char *) dataPtr);
		dump_c2user_data( dataPtr, (char *) b3Configuration_faxg3Ptr +
			b3Configuration_faxg3Ptr->length,
			8, "Head Line", descLen, 1, -1);
	    };
	    break;
	default:
	    b3ConfigurationPtr = (struct b3config *)
		    skip_struct( msg, (char *) b2ConfigurationPtr);
	    if ( check_struct( (struct userdata *) b3ConfigurationPtr, msgend,
			4, "B3 Configuration", descLen, 16, 16) )
	    {
		printf("        %-*s: %d\n", descLen - 8, "LIC",
			GET_WORD( b3ConfigurationPtr->lic));
		printf("        %-*s: %d\n", descLen - 8, "HIC",
			GET_WORD( b3ConfigurationPtr->hic));
		printf("        %-*s: %d\n", descLen - 8, "LTC",
			GET_WORD( b3ConfigurationPtr->ltc));
		printf("        %-*s: %d\n", descLen - 8, "HTC",
			GET_WORD( b3ConfigurationPtr->htc));
		printf("        %-*s: %d\n", descLen - 8, "LOC",
			GET_WORD( b3ConfigurationPtr->loc));
		printf("        %-*s: %d\n", descLen - 8, "HOC",
			GET_WORD( b3ConfigurationPtr->hoc));
		printf("        %-*s: %d\n", descLen - 8, "Modulo Mode",
			GET_WORD( b3ConfigurationPtr->moduloMode));
		printf("        %-*s: %d\n", descLen - 8, "Window Size",
			GET_WORD( b3ConfigurationPtr->windowSize));
	    };
	    break;
	};
    };
}

void dump_c2b_name( layer, protocol, descLen)
int layer;
int protocol;
int descLen;
{
    printf("    B%d %-*s: %d (", layer, descLen - 7, "Protocol", protocol);
    switch ( layer )
    {
    case 1:
	switch ( protocol )
	{
	case B1HDLC: printf("64 kBit/s with HDLC framing"); break;
	case B1TRANS: printf("64 kBit/s transparent operation"); break;
	case B1V110TRANS: printf("V.110 asynchronous operation"); break;
	case B1V110HDLC: printf("V.110 asynchronous with HDLC framing"); break;
	case B1FAXG3: printf("T.30 modem for fax group 3"); break;
	case B1HDLCINV: printf("64 kBit/s inverted with HDLC framing"); break;
	case B1HDLC56: printf("56 kBit/s transparent operation"); break;
	case B1MODEM: printf("Modem"); break;
	case B1MODEMASYNC: printf("Modem asynchronous"); break;
	case B1MODEMSYNC: printf("Modem synchronous"); break;
	default: printf("???"); break;
	};
	break;
    case 2:
	switch ( protocol )
	{
	case B2X75: printf("ISO 7776 (X.75 SLP)"); break;
	case B2TRANS: printf("Transparent"); break;
	case B2SDLC: printf("SDLC"); break;
	case B2LAPD: printf("LAPD according Q.921 for D channel X.25"); break;
	case B2T30: printf("T.30 for fax group 3"); break;
	case B2PPP: printf("Point to Point Protocol (PPP)"); break;
	case B2TRANSERR: printf("Transparent (ignoring errors)"); break;
	case B2MODEM: printf("Modem"); break;
	case B2BINTECPPP: printf("BinTec PPP"); break;
	case B2BINTECAPPP: printf("BinTec PPP asynchronous"); break;
	default: printf("???"); break;
	};
	break;
    case 3:
	switch ( protocol )
	{
	case B3TRANS: printf("Transparent"); break;
	case B3T90: printf("T.90NL with compatibility to T.70NL"); break;
	case B3ISO8208: printf("ISO 8208 (X.25 DTE-DTE)"); break;
	case B3X25DCE: printf("X.25 DCE"); break;
	case B3T30: printf("T.30 for fax group 3"); break;
	case B3T30EXT: printf("T.30 ext"); break;
	case B3MODEM: printf("Modem"); break;
	default: printf("???"); break;
	};
	break;
    };
    printf(")\n");
}

void dump_c2ncpi_x25( ptr, msgend, descLen)
struct ncpi2_x25	*ptr;
char			*msgend;
int			descLen;
{
    if ( check_struct( (struct userdata *) ptr, msgend,
		0, "NCPI", descLen, 3, -1) )
    {
	printf("    %-*s: 0x%02x\n", descLen - 4, "Bit field",
					GET_BYTE( ptr->flags));
	printf("    %-*s: %d\n", descLen - 4, "Channel Group",
					GET_BYTE( ptr->chan_group));
	printf("    %-*s: %d\n", descLen - 4, "Channel Number",
					 GET_BYTE( ptr->chan_number));
	printf("    %-*s:", descLen - 4, "PLP Packet");
	if ( ptr->length == 3 )
	{
	    printf(" (empty)\n");
	} else {
	    printf("\n");
	    capi_hexdump_d( ptr->data, ptr->length - 3, 16, 8, descLen);
	};
    };
}

void dump_c2flags( flags)
unsigned int flags;
{
    int priv = 0;

    printf("Flags      : 0x%04x (", flags);
    if ( flags == 0 )
	printf("none");
    if ( flags & 0x0001 ) {
	printf("qualifier bit");
	priv = 1;
    };
    if ( flags & 0x0002 ) {
	if ( priv ) printf(", ");
        printf("more data bit");
	priv = 1;
    };
    if ( flags & 0x0004 ) {
	if ( priv ) printf(", ");
	printf("delivery confirmation bit");
	priv = 1;
    };
    if ( flags & 0x0008 ) {
	if ( priv ) printf(", ");
	printf("expedited data bit");
	priv = 1;
    };
    if ( flags & 0x8000 ) {
	if ( priv ) printf(", ");
	printf("framing error bit");
	priv = 1;
    };
    printf(")\n");
}

void dump_c2reject( rejectValue, descLen)
unsigned short	rejectValue;
int		descLen;
{
    printf("%-*s: %d (", descLen, "Reject", rejectValue);
    switch ( rejectValue )
    {
    case 0: printf("accept the call)\n");			break;
    case 1: printf("ignore the call)\n");			break;
    case 2: printf("reject call, normal call clearing)\n");	break;
    case 3: printf("reject call, user busy)\n");		break;
    case 4: printf("reject call, requested circuit/channel not available)\n"); break;
    case 5: printf("reject call, facility rejected)\n");	break;
    case 6: printf("reject call, channel unacceptable)\n");	break;
    case 7: printf("reject call, incompatible destination)\n");	break;
    case 8: printf("reject call, destination out of order)\n");	break;
    default: printf("??? (unknown reject value))\n");		break;
    };
}

/************************ End of capidump.c ***************************/
