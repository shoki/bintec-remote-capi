/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: CAPI 1.1/2.0 definitions, Remote CAPI definition
 *      Author: Michael Bareiss
 *    $RCSfile: capidef.h,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: include file for CAPI and Remote CAPI
 *    Products: ALL
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/
#ifndef _CAPIDEF_H_
#define _CAPIDEF_H_

#ifdef _cplusplus
extern "C"
{
#endif

#ifndef FAR
#if defined __MSDOS__
#define FAR far
#else
#define FAR
#endif
#endif

#if defined LINUX || defined __MSDOS__ || defined _Windows || \
    defined _WINDOWS  || defined WIN95__
#define MAKROACCESS 	0
#else
#define MAKROACCESS 	1
#endif

/****************************************************************************/

/*------------------------------------------------------*/
/*							*/
/*  CAPI 1.1/2.0 structure access macros		*/
/*							*/
/*------------------------------------------------------*/

#if MAKROACCESS
typedef unsigned char	cDWORD[4];
typedef	unsigned char	cWORD[2];
typedef unsigned char	cBYTE;

# define GET_BYTE(p)	 (p)
# define GET_WORD(p)      (unsigned short)(((p)[1] << 8) | (p)[0])
# define GET_DWORD(p)     (unsigned long)((GET_WORD( (p)+2 ) << 16) | GET_WORD((p)))

# define PUT_BYTE(p, v)	 { (p) = (v); }
# define PUT_WORD(p, v)   { 						\
			    (p)[0] = (v) & 0xff;			\
			    (p)[1] = ((v) >> 8) & 0xff;			\
			 }
# define PUT_DWORD(p, v)  {						\
			    PUT_WORD( (p), (v) & 0xffff );		\
			    PUT_WORD( (p)+2, (v) >> 16 ); 	 	\
			 }

# define CPY_WORD(p, v)   { (p)[0] = (v)[0]; (p)[1] = (v)[1]; }
# define CPY_DWORD(p, v)   { (p)[0] = (v)[0]; (p)[1] = (v)[1];		\
			      (p)[2] = (v)[2]; (p)[3] = (v)[3]; }
#else	/*  no MAKROACCESS  */

#pragma pack(1)

typedef unsigned long	cDWORD;
typedef	unsigned short	cWORD;
typedef unsigned char	cBYTE;

# define GET_BYTE(p)	 (p)
# define GET_WORD(p)     (p)
# define GET_DWORD(p)    (p)

# define PUT_BYTE(p, v)	 (p) = (v)
# define PUT_WORD(p, v)  (p) = (v)
# define PUT_DWORD(p, v) (p) = (v)
# define CPY_WORD(p, v)  (p) = (v)
# define CPY_DWORD(p, v)  (p) = (v)
#endif	/*  MAKROACCESS  */

/*
 * CAPI header access macros
 */
#define GET_LEN( msg )		GET_WORD( (msg)->header.len )
#define GET_APPL( msg )		GET_WORD( (msg)->header.appl )
#define GET_PRIMTYPE( msg )	GET_WORD( (msg)->header.PRIM_type )
#define GET_MESSID( msg )	GET_WORD( (msg)->header.messid )

/*
 * CAPI 1.1 access macros
 */
#define GET_PLCI( msg )		GET_WORD( (msg)->header.ident )
#define GET_NCCI( msg )		GET_WORD( (msg)->header.ident )
#define GET_IDENT( msg )	GET_WORD( (msg)->header.ident )

/*
 * CAPI 2.0 access macros
 */
#define GET_c2PLCI( msg )	GET_DWORD( (msg)->c2header.ident )
#define GET_c2NCCI( msg )	GET_DWORD( (msg)->c2header.ident )
#define GET_c2IDENT( msg )	GET_DWORD( (msg)->c2header.ident )


#ifndef LOBYTE
#define LOBYTE( a )		((a) & 0xff)
#endif
#ifndef HIBYTE
#define HIBYTE( a )		(((a) >> 8) & 0xff)
#endif


/****************************************************************************/

/*------------------------------------------------------*/
/*							*/
/*  UNIX specific					*/
/*							*/
/*------------------------------------------------------*/
#define CAPI_NODE		"/dev/ipimx"
#define CAPI_MODULE		"capi"
#define CAPI_DEVICE		"/dev/capi"

/*------------------------------------------------------*/
/*							*/
/*  UNIX specific					*/
/*  according to CAPI 1.1 for UNIX			*/
/*							*/
/*------------------------------------------------------*/
typedef struct cia_register_params {
    unsigned short	rp_msgcnt;
    unsigned short 	rp_level3cnt;
    unsigned short	rp_datablkcnt;
    unsigned short	rp_datablklen;
} cia_register_params_t;

typedef struct cia_getman {
    char	gm_buf[64];
} cia_getman_t;

typedef struct cia_getvers {
    char	gv_buf[64];
} cia_getvers_t;

typedef struct cia_getserial {
    char 	gs_buf[64];
} cia_getserial_t;

#define CIOC			('c' << 8)
#define CIOCREGISTER		(CIOC |  1)
#define CIOCDEINSTALL		(CIOC |  2)
#define CIOCGETMAN		(CIOC |  3)
#define CIOCGETVERS		(CIOC |  4)
#define CIOCGETSERIAL		(CIOC |  5)
#define CIOCMANUFACTURER	(CIOC |  6)

/*------------------------------------------------------*/
/*                                                      */
/*  UNIX specific                                       */
/*  according to CAPI 2.0 for UNIX                      */
/*                                                      */
/*------------------------------------------------------*/
#define CAPI20_DEVICE		"/dev/capi20"

typedef struct capi_register_params {
    unsigned    level3cnt;
    unsigned    datablkcnt;
    unsigned    datablklen;
} capi_register_params_t;
 
#define C2IOC                    ('C' << 8)
#define CAPI2_REGISTER           (C2IOC |  1)
#define CAPI2_GET_MANUFACTURER   (C2IOC |  6)
#define CAPI2_GET_VERSION        (C2IOC |  7)
#define CAPI2_GET_SERIAL_NUMBER  (C2IOC |  8)
#define CAPI2_GET_PROFILE        (C2IOC |  9)

typedef struct capi_getprofile {
    unsigned short	ncontrl;
    unsigned short	nchannel;
    unsigned long	options;
    unsigned long	b1protocol;
    unsigned long	b2protocol;
    unsigned long	b3protocol;
    unsigned long	reserved[6];
    unsigned long	manufacturer[5];
} capi_getprofile_t;


/*------------------------------------------------------*/
/*                                                      */
/*  BinTec CAPI-Module manufacturer     		*/
/*  specific ioctls					*/
/*                                                      */
/*------------------------------------------------------*/
#define CIOCTRACE		(CIOC | 10)
#define CIOCCONTROLLER		(CIOC | 11)
#define CIOCOPTIONS		(CIOC | 12)
#define CIOCSTATUS		(CIOC | 13)
#define CIOCDEBUGLEVEL		(CIOC | 20)
#define CIOCSETINFOSTRING	(CIOC | 21)


/*------------------------------------------------------*/
/*                                                      */
/*  BinTec CAPI-Module manufacturer     		*/
/*  specific option flags				*/
/*                                                      */
/*------------------------------------------------------*/
#define COMPAT_NODLPDCHECK			0x0001
#define COMPAT_FAXMAXSPEED			0x0002
#define COMPAT_TRANSREVBIT			0x0004
#define COMPAT_X25CALLDBIT			0x0008
#define COMPAT_MAPNOEAZTOZERO			0x0010
#define COMPAT_NOQ931ON				0x0020
#define COMPAT_NOIEDATE				0x0040
#define COMPAT_NOALIVEIND			0x0080
#define COMPAT_ALERTING				0x0100
#define COMPAT_FAXNOECM				0x0200
#define COMPAT_FAXNOHEADER			0x0400
#define COMPAT_FAXNOLOGO			0x0800
#define COMPAT_NOPOWERDETECT                    0x1000
#define COMPAT_NOPMXCRC4                        0x2000
#define COMPAT_NOV42BIS				0x4000
#define COMPAT_INITTRACE			0x8000


/****************************************************************************/

/*------------------------------------------------------*/
/*							*/
/*  MS-DOS specific					*/
/*							*/
/*------------------------------------------------------*/
#define CAPI_INT        		0xf1


/*------------------------------------------------------*/
/*							*/
/*  MS-DOS specific					*/
/*  according to CAPI 1.1 for MSDOS			*/
/*							*/
/*------------------------------------------------------*/
#define CAPI_TSR_REGISTER		(0x01 << 8)
#define CAPI_TSR_RELEASE		(0x02 << 8)
#define CAPI_TSR_PUTMESSAGE		(0x03 << 8)
#define CAPI_TSR_GETMESSAGE		(0x04 << 8)
#define CAPI_TSR_SETSIGNAL		(0x05 << 8)
#define CAPI_TSR_DEINSTALL		(0x06 << 8)
#define CAPI_TSR_GETMANUFACT		(0xf0 << 8)
#define CAPI_TSR_GETVERSION		(0xf1 << 8)
#define CAPI_TSR_GETSERIAL		(0xf2 << 8)
#define CAPI_TSR_GETPROFILE		(0xf3 << 8)
#define CAPI_TSR_MANUFACTURER 		(0xff << 8)

/*------------------------------------------------------*/
/*							*/
/*  MS-DOS specific					*/
/*  according to CAPI 2.0 for MSDOS			*/
/*							*/
/*------------------------------------------------------*/
#define CAPI2_TSR_REGISTER		((20 << 8) | 0x01)
#define CAPI2_TSR_RELEASE		((20 << 8) | 0x02)
#define CAPI2_TSR_PUTMESSAGE		((20 << 8) | 0x03)
#define CAPI2_TSR_GETMESSAGE		((20 << 8) | 0x04)
#define CAPI2_TSR_SETSIGNAL		((20 << 8) | 0x05)
#define CAPI2_TSR_DEINSTALL		((20 << 8) | 0x06)
#define CAPI2_TSR_GETMANUFACT		((20 << 8) | 0xf0)
#define CAPI2_TSR_GETVERSION		((20 << 8) | 0xf1)
#define CAPI2_TSR_GETSERIAL		((20 << 8) | 0xf2)
#define CAPI2_TSR_GETPROFILE		((20 << 8) | 0xf3)
#define CAPI2_TSR_MANUFACTURER 		((20 << 8) | 0xff)





/*------------------------------------------------------*/
/*							*/
/*  CAPI 1.1 msg ranges        				*/
/*							*/
/*------------------------------------------------------*/
#define CAPI1_MAXMSGLEN		180
#define CAPI1_MINMSGLEN		8



/************************************************************************/
/*									*/
/*		1TR6 specific defines 					*/
/*									*/
/************************************************************************/
/*-----------------------------*/
/* CONNECT_REQ bchannel	       */
/*-----------------------------*/
#define CAPI_ANYBCHANNEL		0x83	
#define CAPI_DCHANNEL			0x84

/*-----------------------------*/
/* INFO_REQ infotypes 	       */
/*-----------------------------*/
#define AI_CAUSE	0x0008		/* codeset 0 */
#define AI_DISPLAY	0x0028		/* codeset 0 */
#define AI_DAD		0x0070		/* codeset 0 */
#define AI_UUINFO	0x007e		/* codeset 0 */
#define AI_CHARGE	0x0602		/* codeset 6 */
#define AI_DATE		0x0603		/* codeset 6 */
#define AI_CPS		0x0607		/* codeset 6 */
#define AI_REN		0x0074		/* codeset 0 */


/*-----------------------------*/
/*   Supplementary Services    */
/*-----------------------------*/
#define SS_HOLD_RETRIEVE      	0x0001
#define SS_TERMINAL_PORTABILITY 0x0002
#define SS_ECT                  0x0004
#define SS_3PTY                 0x0008
#define SS_CALL_FORWARD         0x0010
#define SS_CALL_DEFLECTION      0x0020

/*-----------------------------*/
/* CONNECT_REQ, IND services   */ 
/*-----------------------------*/
#define SI_PHONE	1
#define SI_ABSERVICES	2
#define SI_X21		3
#define SI_FAXG4	4
#define SI_BTX		5
#define SI_DATA		7
#define SI_X25		8
#define SI_TELETEX	9
#define SI_MIXEDMODE	10
#define SI_REMOTECTRL	13
#define SI_GRAPHTEL	14
#define SI_VIDEOTEXT	15
#define SI_VIDEOPHONE	16


/************************************************************************/
/*									*/
/*		CAPI 1.1 infomask bit settings				*/
/*									*/
/************************************************************************/
#define CAPI_ICHARGE            0x01L		/* BIT 0  */
#define CAPI_IDATE  		0x02L		/* BIT 1  */
#define CAPI_IDISPLAY           0x04L		/* BIT 2  */
#define CAPI_IUUINFO            0x08L		/* BIT 3  */
#define CAPI_ICAUSE             0x10L		/* BIT 4  */
#define CAPI_ISTATE             0x20L		/* BIT 5  */
#define CAPI_IDESTINATION	0x40L		/* BIT 6  */
#define CAPI_IDTMF		0x80L		/* BIT 7  */
#define CAPI_ISPV               0x40000000L	/* BIT 30 */
#define CAPI_ISUBADDR           0x80000000L	/* BIT 31 */

#define CAPI_INFOMASK		( CAPI_ICHARGE  | CAPI_IDATE    | \
				  CAPI_IDISPLAY | CAPI_IUUINFO  | \
				  CAPI_ICAUSE   | CAPI_ISTATE   | \
				  CAPI_ISPV     | CAPI_ISUBADDR | \
				  CAPI_IDTMF    | CAPI_IDESTINATION)

#define CAPI_SIMASK             0xe7bf
#define CAPI_EAZMASK		0x3ff


#define CAPI_PL_LISTENEAZ	1



/************************************************************************/
/*									*/
/*		CAPI 1.1 error codes					*/
/*									*/
/************************************************************************/
#define CAPI_E_REGISTER			0x1001
#define CAPI_E_APPLICATION		0x1002
#define CAPI_E_MSGLENGTH		0x1003
#define CAPI_E_COMMAND			0x1004
#define CAPI_E_QUEUEFULL		0x1005
#define CAPI_E_NOMSG			0x1006
#define CAPI_E_MSGOVERFLOW		0x1007
#define CAPI_E_DEINSTALL		0x1008
#define CAPI_E_CONTROLLER		0x2001
#define CAPI_E_PLCI			0x2002
#define CAPI_E_NCCI			0x2003
#define CAPI_E_BCHANNEL			0x3101
#define CAPI_E_INFOMASK			0x3102
#define CAPI_E_EAZMASK			0x3103
#define CAPI_E_SIMASK			0x3104
#define CAPI_E_B2PROTO			0x3105
#define CAPI_E_DLPD			0x3106
#define CAPI_E_B3PROTO			0x3107
#define CAPI_E_NCPD			0x3108
#define CAPI_E_NCPI			0x3109
#define CAPI_E_DATAB3FLAGS		0x310a
#define CAPI_E_CONTROLLERFAILED		0x3201
#define CAPI_E_REGCONFLICT		0x3202
#define CAPI_E_CMDNOTSUPPORTED		0x3203
#define CAPI_E_PLCIACT			0x3204
#define CAPI_E_NCCIACT			0x3205
#define CAPI_E_B2NOTSUPPORT		0x3206
#define CAPI_E_B2STATE   		0x3207
#define CAPI_E_B3NOTSUPPORT		0x3208
#define CAPI_E_B3STATE			0x3209
#define CAPI_E_DLPDPARA			0x320a
#define CAPI_E_NCPDPARA			0x320b
#define CAPI_E_DATALEN			0x320d
#define CAPI_E_DTMF			0x320e
#define CAPI_E_NOL1			0x3301
#define CAPI_E_NOL2			0x3302
#define CAPI_E_SETUPBCHANLAYER1         0x3303
#define CAPI_E_SETUPBCHANLAYER2         0x3304
#define CAPI_E_ABORTDCHANLAYER1         0x3305
#define CAPI_E_ABORTDCHANLAYER2         0x3306
#define CAPI_E_ABORTDCHANLAYER3         0x3307
#define CAPI_E_ABORTBCHANLAYER1         0x3308
#define CAPI_E_ABORTBCHANLAYER2         0x3309
#define CAPI_E_ABORTBCHANLAYER3         0x330a
#define CAPI_E_REBCHANLAYER3            0x330c
#define CAPI_E_NOFAX                    0x4001
#define CAPI_E_FAXBUSY			0x4002
#define CAPI_E_BADQUALITY		0x4003
#define CAPI_E_BADLINE                  0x4004
#define CAPI_E_TOOSLOW			0x4005
#define CAPI_E_FAXBLOCKED		0x4006
#define CAPI_E_LOCABORT			0x4007
#define CAPI_E_NOANSWER                 0x4008
#define CAPI_E_REMDISC                  0x4009
#define CAPI_E_NOCMD                    0x400a
#define CAPI_E_INCOMPAT                 0x400b
#define CAPI_E_BADDATA                  0x400c
#define CAPI_E_PROTO                    0x400d

/************************************************************************/
/*                                                                      */
/*              CAPI 2.0 error codes                                    */
/*                                                                      */
/************************************************************************/
#define CAPI2_E_FLAGS_NOT_SUPPORTED_BY_PROTOCOL          0x0002
#define CAPI2_E_ALERT_ALREADY_SENT                       0x0003
 
#define CAPI2_E_REG_TOO_MANY_APPLICATIONS                0x1001
#define CAPI2_E_REG_BLOCKSIZE_TOO_SMALL                  0x1002
#define CAPI2_E_REG_BUFFER_EXCEED_64K                    0x1003
#define CAPI2_E_REG_MSGSIZE_TOO_SMALL                    0x1004
#define CAPI2_E_REG_MAX_NUM_CONN_NOT_SUPPORTED           0x1005
#define CAPI2_E_REG_INTERNAL_BUSY_CONDITION              0x1007
#define CAPI2_E_REG_OS_RESOURCE_ERROR                    0x1008
#define CAPI2_E_REG_CAPI_NOT_INSTALLED                   0x1009
#define CAPI2_E_REG_EXT_EQUIPMENT_NOT_SUPPORTED          0x100a
#define CAPI2_E_REG_ONLY_EXT_EQUIPMENT_SUPPORTED         0x100b
 
#define CAPI2_E_ILLEGAL_APPLICATION                      0x1101
#define CAPI2_E_ILLEGAL_COMMAND                          0x1102
#define CAPI2_E_MSG_QUEUE_FULL                           0x1103
#define CAPI2_E_MSG_QUEUE_EMPTY                          0x1104
#define CAPI2_E_MSG_QUEUE_OVERFLOW                       0x1105
#define CAPI2_E_UNKNOWN_NOTIFICATION_PARAM               0x1106
#define CAPI2_E_INTERNAL_BUSY_CONDITION                  0x1107
#define CAPI2_E_OS_RESOURCE_ERROR                        0x1108
#define CAPI2_E_CAPI_NOT_INSTALLED                       0x1109
#define CAPI2_E_EXT_EQUIPMENT_NOT_SUPPORTED              0x110a
#define CAPI2_E_ONLY_EXT_EQUIPMENT_SUPPORTED             0x110b
 
#define CAPI2_E_OUT_OF_STATE                             0x2001
#define CAPI2_E_ILLEGAL_IDENT                            0x2002
#define CAPI2_E_OUT_OF_PLCI                              0x2003
#define CAPI2_E_OUT_OF_NCCI                              0x2004
#define CAPI2_E_OUT_OF_LISTEN                            0x2005
#define CAPI2_E_OUT_OF_FAX_RESOURCES                     0x2006
#define CAPI2_E_ILLEGAL_MSG_PARAM_CODING                 0x2007
 
#define CAPI2_E_B1_PROTOCOL_NOT_SUPPORTED                0x3001
#define CAPI2_E_B2_PROTOCOL_NOT_SUPPORTED                0x3002
#define CAPI2_E_B3_PROTOCOL_NOT_SUPPORTED                0x3003
#define CAPI2_E_B1_PROTOCOL_PARAM_NOT_SUPPORTED          0x3004
#define CAPI2_E_B2_PROTOCOL_PARAM_NOT_SUPPORTED          0x3005
#define CAPI2_E_B3_PROTOCOL_PARAM_NOT_SUPPORTED          0x3006
#define CAPI2_E_B_PROTOCOL_COMBINATION_NOT_SUPPORTED     0x3007
#define CAPI2_E_NCPI_NOT_SUPPORTED                       0x3008
#define CAPI2_E_CIP_VALUE_UNKNOWN                        0x3009
#define CAPI2_E_FLAGS_NOT_SUPPORTED                      0x300a
#define CAPI2_E_FACILITY_NOT_SUPPORTED                   0x300b
#define CAPI2_E_DATA_LENGTH_NOT_SUPPORTED                0x300c
#define CAPI2_E_RESET_NOT_SUPPORTED                      0x300d
#define CAPI2_E_SERVICE_NOT_SUPPORTED                    0x300e
#define CAPI2_E_SERVICE_WRONG_STATE                      0x3010

 
#define CAPI2_E_PROTOCOL_ERROR_LAYER_1                   0x3301
#define CAPI2_E_PROTOCOL_ERROR_LAYER_2                   0x3302
#define CAPI2_E_PROTOCOL_ERROR_LAYER_3                   0x3303
#define CAPI2_E_ANOTHER_APPL_GOT_THE_CALL                0x3304

#define CAPI2_E_FAX_NO_FAXG3				 0x3311
#define CAPI2_E_FAX_TRAINING_ERROR			 0x3312
#define CAPI2_E_FAX_INCOMPATIBLE		         0x3313
#define CAPI2_E_FAX_REMOTE_ABORT			 0x3314
#define CAPI2_E_FAX_REMOTE_PROCEDURE_ERROR		 0x3315
#define CAPI2_E_FAX_LOCAL_TX_UNDERRUN			 0x3316
#define CAPI2_E_FAX_LOCAL_RX_UNDERRUN			 0x3317
#define CAPI2_E_FAX_LOCAL_ABORT				 0x3318
#define CAPI2_E_FAX_ILLEGAL_CODING			 0x3319

#define CAPI2_E_MDM_NORMAL_END				 0x3500
#define CAPI2_E_MDM_CARRIER_LOST			 0x3501
#define CAPI2_E_MDM_NEGOTIATION_ERROR			 0x3502
#define CAPI2_E_MDM_NO_ANSWER_TO_PROTOCOL_REQUEST	 0x3503
#define CAPI2_E_MDM_ONLY_SYNC_MODE			 0x3504
#define CAPI2_E_MDM_FRAMING_FAILS			 0x3505
#define CAPI2_E_MDM_PROTOCOL_NEGOTIATION_FAILS		 0x3506
#define CAPI2_E_MDM_WRONG_REMOTE_PROTOCOL_REQUEST	 0x3507
#define CAPI2_E_MDM_SYNC_INFO_MISSING			 0x3508
#define CAPI2_E_MDM_REMOTE_NORMAL_END			 0x3509
#define CAPI2_E_MDM_NO_ANSWER_FROM_OTHER_MODEM		 0x350a
#define CAPI2_E_MDM_PROCOL_ERROR			 0x350b
#define CAPI2_E_MDM_COMPRESSION_ERROR			 0x350c
#define CAPI2_E_MDM_TIMEOUT_NO_CONNECT			 0x350d
#define CAPI2_E_MDM_FALLBACK_NOT_ALLOWED		 0x350e
#define CAPI2_E_MDM_NO_MODEM_OR_FAX			 0x350f
#define CAPI2_E_MDM_HANDSHAKE_ERROR			 0x3510
 

/************************************************************************/
/*									*/
/*		CAPI 1.1 primitives					*/
/*									*/
/************************************************************************/
#define CAPI_CONNECT_REQ		0x0002
#define CAPI_CONNECT_CONF		0x0102
#define CAPI_CONNECT_IND		0x0202
#define CAPI_CONNECT_RESP		0x0302
#define CAPI_CONNECTINFO_REQ		0x0009
#define CAPI_CONNECTINFO_CONF		0x0109
#define CAPI_CONNECTACTIVE_IND		0x0203
#define CAPI_CONNECTACTIVE_RESP		0x0303
#define CAPI_DISCONNECT_REQ		0x0004
#define CAPI_DISCONNECT_CONF		0x0104
#define CAPI_DISCONNECT_IND		0x0204
#define CAPI_DISCONNECT_RESP		0x0304
#define CAPI_LISTEN_REQ			0x0005
#define CAPI_LISTEN_CONF		0x0105
#define CAPI_GETPARAMS_REQ		0x0006
#define CAPI_GETPARAMS_CONF		0x0106
#define CAPI_INFO_REQ			0x0007
#define CAPI_INFO_CONF			0x0107
#define CAPI_INFO_IND			0x0207
#define CAPI_INFO_RESP			0x0307
#define CAPI_DATA_REQ			0x0008
#define CAPI_DATA_CONF			0x0108
#define CAPI_DATA_IND			0x0208
#define CAPI_DATA_RESP			0x0308
#define CAPI_SELECTB2_REQ		0x0040
#define CAPI_SELECTB2_CONF		0x0140
#define CAPI_SELECTB3_REQ		0x0080
#define CAPI_SELECTB3_CONF		0x0180
#define CAPI_LISTENB3_REQ		0x0081
#define CAPI_LISTENB3_CONF		0x0181
#define CAPI_CONNECTB3_REQ		0x0082
#define CAPI_CONNECTB3_CONF		0x0182
#define CAPI_CONNECTB3_IND		0x0282
#define CAPI_CONNECTB3_RESP		0x0382
#define CAPI_CONNECTB3ACTIVE_IND	0x0283
#define CAPI_CONNECTB3ACTIVE_RESP	0x0383
#define CAPI_DISCONNECTB3_REQ		0x0084
#define CAPI_DISCONNECTB3_CONF		0x0184
#define CAPI_DISCONNECTB3_IND		0x0284
#define CAPI_DISCONNECTB3_RESP		0x0384
#define CAPI_GETB3PARAMS_REQ		0x0085
#define CAPI_GETB3PARAMS_CONF		0x0185
#define CAPI_DATAB3_REQ			0x0086
#define CAPI_DATAB3_CONF		0x0186
#define CAPI_DATAB3_IND			0x0286
#define CAPI_DATAB3_RESP		0x0386
#define CAPI_RESETB3_REQ		0x0001
#define CAPI_RESETB3_CONF		0x0101
#define CAPI_RESETB3_IND		0x0201
#define CAPI_RESETB3_RESP		0x0301
#define CAPI_HANDSET_IND		0x0287
#define CAPI_HANDSET_RESP		0x0387
#define CAPI_DTMF_REQ			0x000a
#define CAPI_DTMF_CONF			0x010a
#define CAPI_DTMF_IND			0x020a
#define CAPI_DTMF_RESP			0x030a

/************************************************************************/
/*                                                                      */
/*              CAPI 2.0 primitives                                     */
/*                                                                      */
/************************************************************************/
#define CAPI2_ALERT_REQ                  0x8001
#define CAPI2_ALERT_CONF                 0x8101
#define CAPI2_CONNECT_REQ                0x8002
#define CAPI2_CONNECT_CONF               0x8102
#define CAPI2_CONNECT_IND                0x8202
#define CAPI2_CONNECT_RESP               0x8302
#define CAPI2_CONNECTACTIVE_IND          0x8203
#define CAPI2_CONNECTACTIVE_RESP         0x8303
#define CAPI2_DISCONNECT_REQ             0x8004
#define CAPI2_DISCONNECT_CONF            0x8104
#define CAPI2_DISCONNECT_IND             0x8204
#define CAPI2_DISCONNECT_RESP            0x8304
#define CAPI2_LISTEN_REQ                 0x8005
#define CAPI2_LISTEN_CONF                0x8105
#define CAPI2_INFO_REQ                   0x8008
#define CAPI2_INFO_CONF                  0x8108
#define CAPI2_INFO_IND                   0x8208
#define CAPI2_INFO_RESP                  0x8308
#define CAPI2_SELECTB_REQ                0x8041
#define CAPI2_SELECTB_CONF               0x8141
#define CAPI2_FACILITY_REQ               0x8080
#define CAPI2_FACILITY_CONF              0x8180
#define CAPI2_FACILITY_IND               0x8280
#define CAPI2_FACILITY_RESP              0x8380
#define CAPI2_CONNECTB3_REQ              0x8082
#define CAPI2_CONNECTB3_CONF             0x8182
#define CAPI2_CONNECTB3_IND              0x8282
#define CAPI2_CONNECTB3_RESP             0x8382
#define CAPI2_CONNECTB3ACTIVE_IND        0x8283
#define CAPI2_CONNECTB3ACTIVE_RESP       0x8383
#define CAPI2_DISCONNECTB3_REQ           0x8084
#define CAPI2_DISCONNECTB3_CONF          0x8184
#define CAPI2_DISCONNECTB3_IND           0x8284
#define CAPI2_DISCONNECTB3_RESP          0x8384
#define CAPI2_DATAB3_REQ                 0x8086
#define CAPI2_DATAB3_CONF                0x8186
#define CAPI2_DATAB3_IND                 0x8286
#define CAPI2_DATAB3_RESP                0x8386
#define CAPI2_RESETB3_REQ                0x8087
#define CAPI2_RESETB3_CONF               0x8187
#define CAPI2_RESETB3_IND                0x8287
#define CAPI2_RESETB3_RESP               0x8387
#define CAPI2_CONNECTB3T90ACTIVE_IND     0x8288
#define CAPI2_CONNECTB3T90ACTIVE_RESP    0x8388
#define CAPI2_MANUFACT_REQ               0x80ff
#define CAPI2_MANUFACT_CONF              0x81ff
#define CAPI2_MANUFACT_IND               0x82ff
#define CAPI2_MANUFACT_RESP              0x83ff


/************************************************************************/
/*									*/
/*		BinTec specific CAPI primitives				*/	
/*									*/
/************************************************************************/
#define CAPI_CONTROL_REQ		0x00ff
#define CAPI_CONTROL_CONF		0x01ff
#define CAPI_CONTROL_IND		0x02ff
#define CAPI_CONTROL_RESP		0x03ff

#define CAPI_GETPROFILE_REQ		0xffe0
#define CAPI_GETPROFILE_CONF		0xffe1

#define CAPI_ALIVE_IND			0xfff0
#define CAPI_ALIVE_RESP			0xfff1
#define CAPI_REGISTER_REQ		0xfff2
#define CAPI_REGISTER_CONF		0xfff3
#define CAPI_RELEASE_REQ		0xfff4
#define CAPI_RELEASE_CONF		0xfff5
#define CAPI_GETMANUFACT_REQ		0xfffa
#define CAPI_GETMANUFACT_CONF		0xfffb
#define CAPI_GETVERSION_REQ		0xfffc
#define CAPI_GETVERSION_CONF		0xfffd
#define CAPI_GETSERIAL_REQ		0xfffe
#define CAPI_GETSERIAL_CONF		0xffff


/************************************************************************/
/*									*/
/*		Flags for CAPI 1.1 CAPI_DATAB3_REQ/IND			*/	
/*									*/
/************************************************************************/
#define CAPI_QUALIFIER			0x01
#define CAPI_MORE_FLAG			0x02
#define CAPI_DELIVERY			0x04
#define CAPI_ALLFLAGS			(CAPI_QUALIFIER | \
					 CAPI_MORE_FLAG | \
					 CAPI_DELIVERY)


/************************************************************************/
/*									*/
/*		BinTec specific control type values			*/	
/*									*/
/************************************************************************/
#define CTRL_CAPIREC_ON			0x01
#define CTRL_CAPIREC_OFF		0x02
#define CTRL_CAPIREC_PLAY		0x03
#define CTRL_TRACELEVEL			0x04
#define CTRL_BOARD_CONFIG		0x05
#define CTRL_BOARD_LOAD			0x06
#define CTRL_BOARD_UNLOAD		0x07
#define CTRL_LOOPBACK			0x08
#define CTRL_CAPISTATE			0x09
#define CTRL_CMDEXEC			0x0a
#define CTRL_ISDNREC_ON			0x0c
#define CTRL_ISDNREC_OFF		0x0d
#define CTRL_ISDNREC_PLAY		0x0e
#define CTRL_ISDNREC_CLEAR		0x0f
#define CTRL_STATIST			0x10
#define CTRL_EAZMAPPING			0x11
#define CTRL_ISDNREC_TRACE		0x12
#define CTRL_CAPIREC_TRACE		0x13
#define CTRL_CAPI_NEWCALL		0x14
#define CTRL_CAPI_RMVCALL		0x15
#define CTRL_CAPI_STATCALL		0x16
#define CTRL_CAPI_CHGCALL		0x17
#define CTRL_CAPI_L1STATE		0x18
#define CTRL_GETCHALLENGE		0x19
#define CTRL_SETUSER			0x1a

/************************************************************************/
/*									*/
/*		BinTec specific PPP option settings 			*/	
/*									*/
/************************************************************************/
#define PPP_LOC_ALLOW   0x01
#define PPP_LOC_MAND    0x02
#define PPP_LOC_WANT    0x04
#define PPP_LOC_NEG     0x08
#define PPP_RMT_ALLOW   0x10
#define PPP_RMT_MAND    0x20
#define PPP_RMT_WANT    0x40
#define PPP_RMT_NEG     0x80


/************************************************************************/
/*									*/
/*		CAPI 1.1 protocol types					*/	
/*									*/
/************************************************************************/
enum l2prots { 	L2X75		= 0x01, 
		L2HDLCCRC	= 0x02,  
		L2TRANS		= 0x03,
		L2SDLC		= 0x04,	/* not yet implemented */   
		L2X75BTX	= 0x05,
		L2FAX		= 0x06,	
		L2LAPD		= 0x07, 
		L2V110TRANS	= 0x08,
		L2V110SDLC	= 0x09,	/* not yet implemented */  
		L2V110X75	= 0x0a,
		L2TXONLY	= 0x0b,
		L2MODEM		= 0xf0,
		L2V110SYNC	= 0xf1
};

enum l3prots { 	L3T70NL		= 0x01, 
		L3ISO8208	= 0x02,
		L3T90		= 0x03,
		L3TRANS		= 0x04,
		L3T30		= 0x05,
		L3PPP		= 0xf1,
		L3PPPASYNC	= 0xf2
};



/************************************************************************/
/*									*/
/*		CAPI 1.1 userdata structures				*/	
/*									*/
/************************************************************************/
typedef struct userdata {
    cBYTE length;
    cBYTE data[ CAPI1_MAXMSGLEN ];
} userdata_t;

typedef struct telno {
    cBYTE 	length;
    cBYTE 	type;
    cBYTE 	no[32];
} telno_t;

typedef struct dlpd {
    cBYTE     	length;
    cWORD    	data_length;
    cBYTE     	link_addr_a;
    cBYTE     	link_addr_b;
    cBYTE     	modulo_mode;
    cBYTE     	window_size;
    cBYTE     	xid;
} dlpd_t;

typedef struct dlpd_v110 {
    cBYTE     	length;
    cWORD    	data_length;
    cBYTE     	link_addr_a;
    cBYTE     	link_addr_b;
    cBYTE     	modulo_mode;
    cBYTE     	window_size;
    cBYTE     	user_rate;	/* V110 */
    cBYTE     	xid;
} dlpd_v110_t;

typedef struct ncpd_x25 {
    cBYTE     	length;
    cWORD    	lic;
    cWORD    	hic;
    cWORD    	ltc;
    cWORD    	htc;
    cWORD    	loc;
    cWORD    	hoc;
    cBYTE     	modulo_mode;
    /*
     * BinTec specific addon
     */
    cBYTE	dflt_window_size;
} ncpd_x25_t;

typedef struct ncpd_fax {
    cBYTE  	length;
    cBYTE  	resolution;
    cBYTE  	max_speed;
    cBYTE  	format;
    cBYTE  	xmit_level;
    cBYTE  	station_id_length;
} ncpd_fax_t;

typedef struct ncpi_fax {
    cBYTE  	length;
    cBYTE  	resolution;
    cBYTE  	speed;
    cBYTE  	format;
    cBYTE  	pages;
    cBYTE  	receiver_id_length;
} ncpi_fax_t;

/************************************************************************/
/*									*/
/*		CAPI 2.0 userdata structures				*/	
/*									*/
/************************************************************************/
enum b1proto { 	B1HDLC		= 0x00,
		B1TRANS		= 0x01,
		B1V110TRANS	= 0x02,
		B1V110HDLC	= 0x03,
		B1FAXG3		= 0x04,
		B1HDLCINV	= 0x05,
		B1HDLC56	= 0x06,
		B1MODEM		= 0x07,
		B1MODEMASYNC	= 0x08,
		B1MODEMSYNC	= 0x09,
};

enum b2proto {	B2X75		= 0x00,
		B2TRANS		= 0x01,
		B2SDLC		= 0x02,
		B2LAPD		= 0x03,
		B2T30		= 0x04,
		B2PPP		= 0x05,
		B2TRANSERR	= 0x06,
		B2MODEM		= 0x07,
		B2X75V42BIS	= 0x08,
		B2V120ASYNC	= 0x09,
		B2V120ASYNCV42BIS	= 0x0a,
		B2V120TRANS	= 0x0b,
		B2LAPDSAPI	= 0x0c,
		B2BINTECPPP	= 0xf1,	/* BinTec PPP */
		B2BINTECAPPP	= 0xf2,	/* BinTec PPP */
};

enum b3proto {	B3TRANS		= 0x00,
		B3T90		= 0x01,
		B3ISO8208	= 0x02,
		B3X25DCE	= 0x03,
		B3T30		= 0x04,
		B3T30EXT	= 0x05,
		B3MODEM		= 0x07,
};

typedef struct capi_profile {
    cWORD 	ctrl_cnt;
    cWORD	bchan_cnt;
    cDWORD	options;
    cDWORD	b1protocols;
    cDWORD	b2protocols;
    cDWORD 	b3protocols;
    cDWORD	reserved[2];
} capi_profile_t;

typedef struct bprotocol {
    cBYTE 	length;
    cWORD	b1proto;
    cWORD	b2proto;
    cWORD	b3proto;
    cBYTE	structlen;
    /*
     * following:
     * struct B1 configuration
     * struct B2 configuration
     * struct B3 configuration
     */
} bprotocol_t;

typedef struct b1config {
    cBYTE 	length;
    cWORD 	rate;
    cWORD	bpc;
    cWORD	parity;
    cWORD	stopbits;
} b1config_t;

typedef struct b1config_modem {
    cBYTE 	length;
    cWORD 	rate;
    cWORD	bpc;
    cWORD	parity;
    cWORD	stopbits;
    cWORD	options;
    cWORD	negotiation;
} b1config_modem_t;


typedef struct b2config {
    cBYTE 	length;
    cBYTE	addressA;
    cBYTE	addressB;
    cBYTE	moduloMode;
    cBYTE	windowSize;
    cBYTE	xidlen;
} b2config_t;

typedef struct b2config_v42 {
    cBYTE 	length;
    cBYTE	addressA;
    cBYTE	addressB;
    cBYTE	moduloMode;
    cBYTE	windowSize;
    cWORD	direction;
    cWORD	codewords;
    cWORD	maxStrLen;
} b2config_v42_t;

typedef struct b2config_modem {
    cBYTE 	length;
    cWORD	options;
} b2config_modem_t;

typedef struct b3config {
    cBYTE 	length;
    cWORD	lic;
    cWORD	hic;
    cWORD	ltc;
    cWORD	htc;
    cWORD	loc;
    cWORD	hoc;
    cWORD	moduloMode;
    cWORD	windowSize;
} b3config_t;

typedef struct b3config_faxg3 {
    cBYTE 	length;
    cWORD	resolution;
    cWORD	format;
    cBYTE	structlen;
    /*
     *    following:
     *	struct station id
     *	struct head line
     */
} b3config_faxg3_t;

typedef struct b3config_faxg3ext {
    cBYTE 	length;
    cWORD	options;
    cWORD	format;
    cBYTE	structlen;
    /*
     *    following:
     *	struct station id
     *	struct head line
     */
} b3config_faxg3ext_t;

typedef struct subaddr {
    cBYTE	length;
    cBYTE 	type;
    cBYTE	no[32];
} subaddr_t;

typedef struct addinfo {
    cBYTE 	length;
    cBYTE	structlen;
    /*
     *    following:
     *	struct B channel information
     *	struct keypad facility
     *	struct user user data
     *	struct facility data
     */
} addinfo_t;

typedef struct bchaninfo {
    cBYTE	length;
    cWORD	channel;
    userdata_t  chid;
} bchaninfo_t;

typedef struct ncpi2_fax {
    cBYTE	length;
    cWORD	rate;
    cWORD	resolution;
    cWORD	format;
    cWORD	pages;
    cBYTE	structlen;
    /*
     *    following:
     *	id of remote side
     */
} ncpi2_fax_t;

typedef struct ncpi2_faxe {
    cBYTE	length;
    cWORD	rate;
    cWORD	options;
    cWORD	format;
    cWORD	pages;
    cBYTE	structlen;
    /*
     *    following:
     *	id of remote side
     */
} ncpi2_faxe_t;

typedef struct ncpi2_x25 {
    cBYTE	length;
    cBYTE	flags;
    cBYTE	chan_group;
    cBYTE	chan_number;
    cBYTE	data[180];
    /*
     *    following
     *	x.25 plp packet
     */
} ncpi2_x25_t;

typedef struct charge {
    cBYTE	length;
    cDWORD	value;
} charge_t;

typedef struct facreq {
    cBYTE	length;
    cWORD	function;
    cBYTE	structlen;
    cBYTE	data;
}facreq_t;

typedef struct faccon {
    cBYTE	length;
    cWORD	dtmf_info;
}faccon_t;

typedef struct dtmfreq {
    cBYTE	length;
    cWORD	function;
    cWORD	tone;
    cWORD	gap;
    cBYTE	structlen;
    cBYTE	data[1];
}dtmfreq_t;

typedef struct ss_facreq {
    cBYTE	length;
    cWORD	function;
    cBYTE       structlen;
    cDWORD      value;
}ss_facreq_t;

typedef struct ss_faccon {
    cBYTE      length;
    cWORD      function;
    cBYTE      structlen;
    cWORD      ssinfo;
    cDWORD     supservices;
}ss_faccon_t;

typedef struct ss_faccdreq {
    cBYTE      length;
    cWORD      function;
    cBYTE      structlen;
    cWORD      presentation;
    cBYTE      number[1];
}ss_faccdreq_t;

typedef struct ss_facawsreq {
    cBYTE      length;
    cWORD      function;
    cBYTE      structlen;
    cDWORD     handle;
    cWORD      cfmode;
    cWORD      service;
    cBYTE      numbers[1];
}ss_facawsreq_t;
    
    
    

/************************************************************************/
/*									*/
/*		Flags for CAPI 2.0 CAPI_DATAB3_REQ/IND			*/	
/*									*/
/************************************************************************/
#define CAPI2_QUALIFIER			(1 <<  0)
#define CAPI2_MORE_FLAG			(1 <<  1)
#define CAPI2_DELIVERY			(1 <<  2)
#define CAPI2_EXPEDITED			(1 <<  3)
#define CAPI2_FRAMINGERR		(1 << 15)
#define CAPI2_ALLFLAGS			(CAPI2_QUALIFIER | \
					 CAPI2_MORE_FLAG | \
					 CAPI2_DELIVERY  | \
					 CAPI2_EXPEDITED)

/************************************************************************/
/*									*/
/*		CAPI 2.0 Infomask Bit settings				*/
/*									*/
/************************************************************************/
#define CAPI2_ICAUSE		(1 << 0)
#define CAPI2_ITIME		(1 << 1)
#define CAPI2_IDISPLAY		(1 << 2)
#define CAPI2_IUUINFO		(1 << 3)
#define CAPI2_IPROGRESS		(1 << 4)
#define CAPI2_IFACILITY		(1 << 5)
#define CAPI2_ICHARGE		(1 << 6)
#define CAPI2_ICPN		(1 << 7)
#define CAPI2_ICHI		(1 << 8)
#define CAPI2_EARLYB3		(1 << 9)
#define CAPI2_IREN		(1 << 10)

/************************************************************************/
/*									*/
/*		CAPI 2.0 profile options Bit settings			*/
/*									*/
/************************************************************************/
#define CAPI2_P_INT		(1 << 0)
#define CAPI2_P_EXT		(1 << 1)
#define CAPI2_P_HANDSET		(1 << 2)
#define CAPI2_P_DTMF		(1 << 3)
#define CAPI2_P_SUPSERV		(1 << 4)
#define CAPI2_P_LEASED		(1 << 5)
#define CAPI2_P_BCHAN		(1 << 6)
#define CAPI2_P_LINTCON		(1 << 7)

/************************************************************************/
/*									*/
/*		BinTec specific CAPI data structures			*/	
/*									*/
/*		only used in conjunction with the active boards		*/
/*		BRI, BRI-4, PMX running BOSS and CAPI			*/
/*									*/
/************************************************************************/
/* this is not a real message, only an identifier for DOS CAPI.EXE */
#ifdef __MSDOS__
typedef struct capiinitpara {
    char		length;
    char   		identifier[64];
    unsigned		memaddr;
    char		intr;
    char		type;
    char		contrl;
    unsigned long 	contrlmask;
} capiinitpara_t;
#endif

#if defined BOSS     || defined __MSDOS__ || \
    defined _WINDOWS || defined _MSDOS    || \
    defined MACOS    || defined WIN95__

typedef struct capiconfig {
    cBYTE	length;	
    cBYTE    	protocol[32];
    cWORD	teid;
    cWORD	b3pl;
    cWORD	t3id;
    cWORD	contrl;
    cBYTE    	bindaddr[32];
    cDWORD	time;
    cDWORD	flags;
    cBYTE	spid1[20];
    cBYTE	spid2[20];
} capiconfig_t;

typedef struct traceopen {
    cBYTE	length;	
    cBYTE	contrl;
    cBYTE 	channel;
    cBYTE	dummy;
    cWORD	maxlen;		/* -1 if not specified */
    cDWORD	alloclen;
} traceopen_t;

typedef struct tracedata {
    cBYTE 	length;
    cBYTE  	blknum;
    cDWORD	timer;
    cDWORD	ppa;
    cDWORD	event;
    cDWORD	inout;
#if defined __MSDOS__ || defined _MSDOS || defined _WINDOWS
    char far *  dataPtr;
#else
    cDWORD 	dataPtr;
#endif
    cWORD 	datalen;
} tracedata_t;

typedef struct apistatist {
    cBYTE	length;
    cBYTE	contrl;		/* capi boardnumber */
    cBYTE 	usedbchannels;
    cBYTE 	l1stat;
    cDWORD	xpkts[3];	/* packets transmit */
    cDWORD	rpkts[3];	/* packets received */
    cDWORD	xerrs[3];	/* errors transmit */
    cDWORD	rerrs[3];	/* errors received */	

    cDWORD	xthr[2];	/* throughput send */
    cDWORD	rthr[2];	/* throughput receive */
    cWORD	plci[2]; 	/* plci with used bchan */
    cBYTE	pstate[2]; 	/* plci state */
    cDWORD	incon[2];	/* number of incoming */
    cDWORD	outcon[2];	/* number of outgoing */
} apistatist_t;

typedef struct cmuxcall {
    cBYTE	length;
    cDWORD	ident;
    cDWORD	callref;
    cDWORD	age;
    cBYTE	type;
    cBYTE	state;
    cBYTE	channel;
    cBYTE	service;
    cBYTE	addinfo;
    cDWORD	charge;
    cDWORD	rcvPackets;
    cDWORD	rcvOctets;
    cDWORD	rcvErrors;
    cDWORD	txPackets;
    cDWORD	txOctets;
    cDWORD	txErrors;
    cBYTE	rmtNumber;
    /*
     * structs
     * cBYTE	rmtSubaddr;
     * cBYTE	locNumber;
     * cBYTE	locSubaddr;
     * cBYTE	bc;
     * cBYTE	llc;
     * cBYTE	hlc;
     * cBYTE	chargeinfo;
     */
} cmuxcall_t;

typedef struct cmuxl1state {
    cBYTE	length;
    cBYTE	state;
} cmuxl1state_t;

typedef struct eazmapping {
    cBYTE 	length;
    cBYTE 	contrl;
    cBYTE 	eaz;
    cBYTE 	telnolen;
} eazmapping_t;
#endif	/*  BOSS, WINDOWS, MSDOS  */




/************************************************************************/
/*									*/
/*		BinTec specific CAPI message structures			*/	
/*									*/
/*		only used in conjunction with the active boards		*/
/*		BRI, BRI-4, PMX running BOSS and CAPI			*/
/*									*/
/************************************************************************/
typedef struct CAPI_register_req {
    cWORD	len;
    cWORD	appl;		/* applid from the pc */
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD 	buffer; 
    cWORD	nmess;
    cWORD	nconn;
    cWORD	ndblock;
    cWORD	dblocksiz;
    cBYTE	version;
} CAPI_register_req_t;
 
typedef struct CAPI_register_conf {
    cWORD	len;
    cWORD	appl;           /* registered appl */
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	info;
} CAPI_register_conf_t;
 
typedef struct CAPI_release_req {
    cWORD	len;
    cWORD	appl;		/* appl to release */
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	relappl;	/* appl to release */		
} CAPI_release_req_t;
 
typedef struct CAPI_release_conf {
    cWORD	len;
    cWORD	appl;           /* appl to release */
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	info;           /* for error reports */
} CAPI_release_conf_t;

typedef struct CAPI_getmanufact_req {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI_getmanufact_req_t;

typedef struct CAPI_getmanufact_conf {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cBYTE 	structlen;
} CAPI_getmanufact_conf_t;

typedef struct CAPI_getversion_req {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI_getversion_req_t;

typedef struct CAPI_getversion_conf {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	version;
    cBYTE  	structlen;
} CAPI_getversion_conf_t;

typedef struct CAPI_getserial_req {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI_getserial_req_t;

typedef struct CAPI_getserial_conf {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cBYTE 	structlen;
} CAPI_getserial_conf_t;

typedef struct CAPI_getprofile_req {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
    cWORD 	ncontrl;
} CAPI_getprofile_req_t;

typedef struct CAPI_getprofile_conf {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	info;
    cWORD 	ncontrl;
    cWORD 	nchannel;
    cDWORD 	options;
    cDWORD 	b1protocol;
    cDWORD 	b2protocol;
    cDWORD 	b3protocol;
    cBYTE	reserved[24];
    cBYTE	manufacturer[20];
} CAPI_getprofile_conf_t;


typedef struct CAPI_control_req {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	contrl;
    cWORD	type;
    cBYTE  	structlen;
} CAPI_control_req_t;

typedef struct CAPI_control_conf {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	contrl;
    cWORD	type;
    cWORD	info;
} CAPI_control_conf_t;

typedef struct CAPI_control_conf_ex {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	contrl;
    cWORD	type;
    cWORD	info;
    cBYTE  	structlen;
} CAPI_control_conf_ex_t;

typedef struct CAPI_control_ind {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	contrl;
    cWORD	type;
    cBYTE  	structlen;
} CAPI_control_ind_t;

typedef struct CAPI_control_resp {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	contrl;
    cWORD	type;
    cBYTE  	structlen;
} CAPI_control_resp_t;
 
typedef struct CAPI_alive_ind {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI_alive_ind_t;

typedef struct CAPI_alive_resp {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI_alive_resp_t;

 

/************************************************************************/
/*									*/
/*		CAPI 1.1 message structures				*/	
/*									*/
/************************************************************************/
typedef struct CAPI_connect_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cBYTE 	contrl;
    cBYTE 	channel;
    cDWORD	info_mask;
    cBYTE 	dst_service;
    cBYTE 	dst_addinfo;
    cBYTE 	src_eaz;

    cBYTE 	telnolen;
} CAPI_connect_req_t;

typedef struct CAPI_connect_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;	
    cWORD	info;
} CAPI_connect_conf_t;

typedef struct CAPI_connect_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	contrl;
    cBYTE 	dst_service;
    cBYTE 	dst_addinfo;
    cBYTE 	dst_eaz;

    cBYTE 	telnolen;
} CAPI_connect_ind_t;

typedef struct CAPI_connect_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	reject;	
} CAPI_connect_resp_t;

typedef struct CAPI_connectinfo_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	telnolen;
} CAPI_connectinfo_req_t;

typedef struct CAPI_connectinfo_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_connectinfo_conf_t;

typedef struct CAPI_connectactive_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	telnolen;
} CAPI_connectactive_ind_t;

typedef struct CAPI_connectactive_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_connectactive_resp_t;

typedef struct CAPI_disconnect_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	cause;
} CAPI_disconnect_req_t;

typedef struct CAPI_disconnect_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_disconnect_conf_t;

typedef struct CAPI_disconnect_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_disconnect_ind_t;

typedef struct CAPI_disconnect_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_disconnect_resp_t;

typedef struct CAPI_listen_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cBYTE  	contrl;
    cDWORD	info_mask;
    cWORD	eaz_mask;
    cWORD	service_mask;
} CAPI_listen_req_t;

typedef struct CAPI_listen_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cBYTE 	contrl;
    cWORD	info;
} CAPI_listen_conf_t;

typedef struct CAPI_getparams_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_getparams_req_t;

typedef struct CAPI_getparams_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	contrl;
    cBYTE 	channel;
    cWORD	info;
    cBYTE 	linkcnt;
    cBYTE 	service;
    cBYTE 	addinfo;
    cBYTE 	eaz;
    cBYTE 	telnolen;
} CAPI_getparams_conf_t;

typedef struct CAPI_info_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cDWORD	info_mask;
} CAPI_info_req_t;

typedef struct CAPI_info_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_info_conf_t;

typedef struct CAPI_info_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info_number;
    cBYTE 	infolen;
} CAPI_info_ind_t;

typedef struct CAPI_info_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_info_resp_t;

typedef struct CAPI_data_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	structlen;
} CAPI_data_req_t;

typedef struct CAPI_data_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_data_conf_t;

typedef struct CAPI_data_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	structlen;
} CAPI_data_ind_t;

typedef struct CAPI_data_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_data_resp_t;

typedef struct CAPI_selectb2_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE  	proto;
    cBYTE  	dlpdlen;
} CAPI_selectb2_req_t;

typedef struct CAPI_selectb2_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_selectb2_conf_t;

typedef struct CAPI_selectb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE  	proto;
    cBYTE  	ncpdlen;
} CAPI_selectb3_req_t;

typedef struct CAPI_selectb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_selectb3_conf_t;

typedef struct CAPI_listenb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_listenb3_req_t;

typedef struct CAPI_listenb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_listenb3_conf_t;

typedef struct CAPI_connectb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	ncpilen;
} CAPI_connectb3_req_t;

typedef struct CAPI_connectb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	ncci;
    cWORD	info;
} CAPI_connectb3_conf_t;

typedef struct CAPI_connectb3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	plci;
    cBYTE 	ncpilen;
} CAPI_connectb3_ind_t;

typedef struct CAPI_connectb3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cBYTE  	reject;
    cBYTE  	ncpilen;
} CAPI_connectb3_resp_t;

typedef struct CAPI_connectb3active_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cBYTE  	ncpilen;
} CAPI_connectb3active_ind_t;

typedef struct CAPI_connectb3active_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
} CAPI_connectb3active_resp_t;

typedef struct CAPI_disconnectb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cBYTE  	ncpilen;
} CAPI_disconnectb3_req_t;

typedef struct CAPI_disconnectb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	info;
} CAPI_disconnectb3_conf_t;

typedef struct CAPI_disconnectb3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	info;
    cBYTE  	ncpilen;
} CAPI_disconnectb3_ind_t;

typedef struct CAPI_disconnectb3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
} CAPI_disconnectb3_resp_t;

typedef struct CAPI_getb3params_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
} CAPI_getb3params_req_t;

typedef struct CAPI_getb3params_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	plci;
    cWORD	info;
} CAPI_getb3params_conf_t;

typedef struct CAPI_datab3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	datalen;
#ifdef __MSDOS__
    char far *	data;
#else
    cDWORD 	data;
#endif
    cBYTE  	blknum;
    cWORD	flags;
} CAPI_datab3_req_t;

typedef struct CAPI_datab3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cBYTE  	blknum;
    cWORD	info;
} CAPI_datab3_conf_t;

typedef struct CAPI_datab3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	datalen;
#ifdef __MSDOS__
    char far *	data;
#else
    cDWORD	data;
#endif
    cBYTE  	blknum;
    cWORD	flags;
} CAPI_datab3_ind_t;

typedef struct CAPI_datab3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cBYTE  	blknum;
} CAPI_datab3_resp_t;

typedef struct CAPI_resetb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
} CAPI_resetb3_req_t;

typedef struct CAPI_resetb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
    cWORD	info;
} CAPI_resetb3_conf_t;

typedef struct CAPI_resetb3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
} CAPI_resetb3_ind_t;

typedef struct CAPI_resetb3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ncci;
} CAPI_resetb3_resp_t;

typedef struct CAPI_handset_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	contrl;
    cBYTE 	state;
} CAPI_handset_ind_t;

typedef struct CAPI_handset_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_handset_resp_t;

typedef struct CAPI_dtmf_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	tonedurat;
    cWORD	gapdurat;
    cBYTE 	dtmflen;
} CAPI_dtmf_req_t;

typedef struct CAPI_dtmf_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cWORD	info;
} CAPI_dtmf_conf_t;

typedef struct CAPI_dtmf_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
    cBYTE 	dtmflen;
} CAPI_dtmf_ind_t;

typedef struct CAPI_dtmf_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	plci;
} CAPI_dtmf_resp_t;


typedef struct CAPI_sheader_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI_sheader_t;

typedef struct CAPI_header_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cWORD	ident;
} CAPI_header_t;

typedef struct CAPI_cheader_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cBYTE 	contrl;
} CAPI_cheader_t;

/************************************************************************/
/*									*/
/*		CAPI 2.0 message structures				*/	
/*									*/
/************************************************************************/

typedef struct CAPI2_alert_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cBYTE	structlen;
    /*
     * following:
     * struct additional info
     */
} CAPI2_alert_req_t;

typedef struct CAPI2_alert_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cWORD 	info;
} CAPI2_alert_conf_t;

typedef struct CAPI2_connect_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cWORD	cip_value;
    cBYTE	structlen;
    /*
     *    following:
     *	struct called party number
     *	struct calling party number
     *	struct called party subaddress
     *	struct calling party subaddress
     *	struct B protocol
     *	struct bc
     *	struct llc
     *	struct hlc
     *	struct additional info
     */
} CAPI2_connect_req_t;

typedef struct CAPI2_connect_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cWORD 	info;
} CAPI2_connect_conf_t;

typedef struct CAPI2_connect_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cWORD	cip_value;
    cBYTE	structlen;
    /*
     *    following:
     *	struct called party number
     *	struct calling party number
     *	struct called party subaddress
     *	struct calling party subaddress
     *	struct bc
     *	struct llc
     *	struct hlc
     *	struct additional info
     */
} CAPI2_connect_ind_t;

typedef struct CAPI2_connect_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cWORD	reject;
    cBYTE	structlen;
    /*
     *    following:
     *	struct B protocol
     *	struct connected number
     *	struct connected subaddress
     *	struct llc
     *	struct additional info
     */
} CAPI2_connect_resp_t;

typedef struct CAPI2_connectactive_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct connected number
     *	struct connected subaddress
     *	struct llc
     */
} CAPI2_connectactive_ind_t;

typedef struct CAPI2_connectactive_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD 	plci;
} CAPI2_connectactive_resp_t;

typedef struct CAPI2_connectb3active_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_connectb3active_ind_t;

typedef struct CAPI2_connectb3active_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD 	ncci;
} CAPI2_connectb3active_resp_t;

typedef struct CAPI2_connectb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_connectb3_req_t;

typedef struct CAPI2_connectb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD	info;
} CAPI2_connectb3_conf_t;

typedef struct CAPI2_connectb3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_connectb3_ind_t;

typedef struct CAPI2_connectb3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD	reject;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_connectb3_resp_t;

typedef struct CAPI2_connectb3t90active_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_connectb3t90active_ind_t;

typedef struct CAPI2_connectb3t90active_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
} CAPI2_connectb3t90active_resp_t;

typedef struct CAPI2_datab3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
#ifndef __MSDOS__
    cDWORD	dataptr;
#else
    char far *	dataptr;
#endif
    cWORD	datalen;
    cWORD	handle;
    cWORD	flags;
} CAPI2_datab3_req_t;

typedef struct CAPI2_datab3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD	handle;
    cWORD	info;
} CAPI2_datab3_conf_t;

typedef struct CAPI2_datab3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
#ifndef __MSDOS__
    cDWORD	dataptr;
#else
    char far *	dataptr;
#endif
    cWORD	datalen;
    cWORD	handle;
    cWORD	flags;
} CAPI2_datab3_ind_t;

typedef struct CAPI2_datab3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD	handle;
} CAPI2_datab3_resp_t;

typedef struct CAPI2_disconnectb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_disconnectb3_req_t;

typedef struct CAPI2_disconnectb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD	info;
} CAPI2_disconnectb3_conf_t;

typedef struct CAPI2_disconnectb3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD	reason_b3;
    cBYTE	structlen;
    /*
     *    following:
     *	struct ncpi
     */
} CAPI2_disconnectb3_ind_t;

typedef struct CAPI2_disconnectb3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
} CAPI2_disconnectb3_resp_t;

typedef struct CAPI2_disconnect_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cBYTE	structlen;
    /*
     *    following:
     *	struct additional info
     */
} CAPI2_disconnect_req_t;

typedef struct CAPI2_disconnect_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cWORD	info;
} CAPI2_disconnect_conf_t;

typedef struct CAPI2_disconnect_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD 	plci;
    cWORD	reason;
} CAPI2_disconnect_ind_t;

typedef struct CAPI2_disconnect_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
} CAPI2_disconnect_resp_t;

typedef struct CAPI2_facility_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cWORD	selector;
    cBYTE	structlen;
    /*
     *    following:
     *	struct facility request parameter
     */
} CAPI2_facility_req_t;

typedef struct CAPI2_facility_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cWORD	info;
    cWORD	selector;
    cBYTE	structlen;
    /*
     *    following:
     *	struct facility confirmation parameter
     */
} CAPI2_facility_conf_t;

typedef struct CAPI2_facility_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cWORD	selector;
    cBYTE	structlen;
    /*
     *    following:
     *	struct facility indication parameter
     */
} CAPI2_facility_ind_t;

typedef struct CAPI2_facility_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cWORD	selector;
    cBYTE	structlen;
    /*
     *    following:
     *	struct facility response parameter
     */
} CAPI2_facility_resp_t;

typedef struct CAPI2_info_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cBYTE	structlen;
    /*
     *    following:
     *	struct called party number
     *	struct additional info
     */
} CAPI2_info_req_t;

typedef struct CAPI2_info_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cWORD	info;
} CAPI2_info_conf_t;

typedef struct CAPI2_info_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
    cWORD	info;
    cBYTE	structlen;
    /*
     *    following:
     *	struct info element 
     */
} CAPI2_info_ind_t;

typedef struct CAPI2_info_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
} CAPI2_info_resp_t;

typedef struct CAPI2_listen_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cDWORD	info_mask;
    cDWORD	cip_mask;
    cDWORD	cip_mask2;
    cBYTE	structlen;
    /*
     *    following:
     *	struct calling party number
     *	struct calling party subaddress
     */
} CAPI2_listen_req_t;

typedef struct CAPI2_listen_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cWORD	info;
} CAPI2_listen_conf_t;

typedef struct CAPI2_manufact_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cDWORD	manuid;
    cBYTE	structlen;
    /*
     *   following:
     *	struct manufacturer specific data
     */
} CAPI2_manufact_req_t;

typedef struct CAPI2_manufact_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cDWORD	manuid;
    cBYTE	structlen;
    /*
     *   following:
     *	struct manufacturer specific data
     */
} CAPI2_manufact_conf_t;

typedef struct CAPI2_manufact_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cDWORD	manuid;
    cBYTE	structlen;
    /*
     *   following:
     *	struct manufacturer specific data
     */
} CAPI2_manufact_ind_t;

typedef struct CAPI2_manufact_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	contrl;
    cDWORD	manuid;
    cBYTE	structlen;
    /*
     *   following:
     *	struct manufacturer specific data
     */
} CAPI2_manufact_resp_t;

typedef struct CAPI2_resetb3_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cBYTE	structlen;
    /*
     *   following:
     *	struct ncpi
     */
} CAPI2_resetb3_req_t;

typedef struct CAPI2_resetb3_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cWORD 	info;
} CAPI2_resetb3_conf_t;

typedef struct CAPI2_resetb3_ind_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
    cBYTE	structlen;
    /*
     *   following:
     *	struct ncpi
     */
} CAPI2_resetb3_ind_t;

typedef struct CAPI2_resetb3_resp_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ncci;
} CAPI2_resetb3_resp_t;

typedef struct CAPI2_selectb_req_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cBYTE	structlen;
    /*
     *   following:
     *	struct B protocol
     */
} CAPI2_selectb_req_t;

typedef struct CAPI2_selectb_conf_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	plci;
    cWORD	info;
} CAPI2_selectb_conf_t;

typedef struct CAPI2_sheader_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;
} CAPI2_sheader_t;

typedef struct CAPI2_header_s {
    cWORD	len;
    cWORD	appl;
    cWORD	PRIM_type;
    cWORD	messid;

    cDWORD	ident;
} CAPI2_header_t;

typedef union CAPI_primitives {
    cBYTE              			msg[1024]; 
    /*
     * CAPI 1.1 messages
     */
    CAPI_sheader_t			sheader;
    CAPI_cheader_t			cheader;
    CAPI_header_t			header;

    CAPI_connect_req_t			connect_req;
    CAPI_connect_conf_t			connect_conf;
    CAPI_connect_ind_t			connect_ind;
    CAPI_connect_resp_t			connect_resp;
    CAPI_connectinfo_req_t		connectinfo_req;
    CAPI_connectinfo_conf_t		connectinfo_conf;
    CAPI_connectactive_ind_t		connectactive_ind;
    CAPI_connectactive_resp_t		connectactive_resp;
    CAPI_disconnect_req_t		disconnect_req;
    CAPI_disconnect_conf_t		disconnect_conf;
    CAPI_disconnect_ind_t		disconnect_ind;
    CAPI_disconnect_resp_t		disconnect_resp;
    CAPI_listen_req_t			listen_req;
    CAPI_listen_conf_t			listen_conf;
    CAPI_getparams_req_t		getparams_req;
    CAPI_getparams_conf_t		getparams_conf;
    CAPI_info_req_t			info_req;
    CAPI_info_conf_t			info_conf;
    CAPI_info_ind_t			info_ind;
    CAPI_info_resp_t			info_resp;
    CAPI_data_req_t			data_req;
    CAPI_data_conf_t			data_conf;
    CAPI_data_ind_t			data_ind;
    CAPI_data_resp_t			data_resp;
    CAPI_selectb2_req_t			selectb2_req;
    CAPI_selectb2_conf_t		selectb2_conf;
    CAPI_selectb3_req_t			selectb3_req;
    CAPI_selectb3_conf_t		selectb3_conf;
    CAPI_listenb3_req_t			listenb3_req;
    CAPI_listenb3_conf_t		listenb3_conf;
    CAPI_connectb3_req_t		connectb3_req;
    CAPI_connectb3_conf_t		connectb3_conf;
    CAPI_connectb3_ind_t		connectb3_ind;
    CAPI_connectb3_resp_t		connectb3_resp;
    CAPI_connectb3active_ind_t		connectb3active_ind;
    CAPI_connectb3active_resp_t		connectb3active_resp;
    CAPI_disconnectb3_req_t		disconnectb3_req;
    CAPI_disconnectb3_conf_t		disconnectb3_conf;
    CAPI_disconnectb3_ind_t		disconnectb3_ind;
    CAPI_disconnectb3_resp_t		disconnectb3_resp;
    CAPI_getb3params_req_t		getb3params_req;
    CAPI_getb3params_conf_t		getb3params_conf;
    CAPI_datab3_req_t			datab3_req;
    CAPI_datab3_conf_t			datab3_conf;
    CAPI_datab3_ind_t			datab3_ind;
    CAPI_datab3_resp_t			datab3_resp;
    CAPI_resetb3_req_t			resetb3_req;
    CAPI_resetb3_conf_t			resetb3_conf;
    CAPI_resetb3_ind_t			resetb3_ind;
    CAPI_resetb3_resp_t			resetb3_resp;
    CAPI_handset_ind_t			handset_ind;
    CAPI_handset_resp_t			handset_resp;
    CAPI_dtmf_req_t			dtmf_req;
    CAPI_dtmf_conf_t			dtmf_conf;
    CAPI_dtmf_ind_t			dtmf_ind;
    CAPI_dtmf_resp_t			dtmf_resp;

    /*
     * CAPI 2.0 messages
     */
    CAPI2_sheader_t                     c2sheader;
    CAPI2_header_t                      c2header;
    CAPI2_alert_req_t                   c2alert_req;
    CAPI2_alert_conf_t                  c2alert_conf;
    CAPI2_connect_req_t                 c2connect_req;
    CAPI2_connect_conf_t                c2connect_conf;
    CAPI2_connect_ind_t                 c2connect_ind;
    CAPI2_connect_resp_t                c2connect_resp;
    CAPI2_connectactive_ind_t           c2connectactive_ind;
    CAPI2_connectactive_resp_t          c2connectactive_resp;
    CAPI2_disconnect_req_t              c2disconnect_req;
    CAPI2_disconnect_conf_t             c2disconnect_conf;
    CAPI2_disconnect_ind_t              c2disconnect_ind;
    CAPI2_disconnect_resp_t             c2disconnect_resp;
    CAPI2_connectb3_req_t               c2connectb3_req;
    CAPI2_connectb3_conf_t              c2connectb3_conf;
    CAPI2_connectb3_ind_t               c2connectb3_ind;
    CAPI2_connectb3_resp_t              c2connectb3_resp;
    CAPI2_connectb3active_ind_t         c2connectb3active_ind;
    CAPI2_connectb3active_resp_t        c2connectb3active_resp;
    CAPI2_connectb3t90active_ind_t      c2connectb3t90active_ind;
    CAPI2_connectb3t90active_resp_t     c2connectb3t90active_resp;
    CAPI2_disconnectb3_req_t            c2disconnectb3_req;
    CAPI2_disconnectb3_conf_t           c2disconnectb3_conf;
    CAPI2_disconnectb3_ind_t            c2disconnectb3_ind;
    CAPI2_disconnectb3_resp_t           c2disconnectb3_resp;
    CAPI2_info_req_t                    c2info_req;
    CAPI2_info_conf_t                   c2info_conf;
    CAPI2_info_ind_t                    c2info_ind;
    CAPI2_info_resp_t                   c2info_resp;
    CAPI2_resetb3_req_t                 c2resetb3_req;
    CAPI2_resetb3_conf_t                c2resetb3_conf;
    CAPI2_resetb3_ind_t                 c2resetb3_ind;
    CAPI2_resetb3_resp_t                c2resetb3_resp;
    CAPI2_datab3_req_t                  c2datab3_req;
    CAPI2_datab3_conf_t                 c2datab3_conf;
    CAPI2_datab3_ind_t                  c2datab3_ind;
    CAPI2_datab3_resp_t                 c2datab3_resp;
    CAPI2_facility_req_t                c2facility_req;
    CAPI2_facility_conf_t               c2facility_conf;
    CAPI2_facility_ind_t                c2facility_ind;
    CAPI2_facility_resp_t               c2facility_resp;
    CAPI2_listen_req_t                  c2listen_req;
    CAPI2_listen_conf_t                 c2listen_conf;
    CAPI2_manufact_req_t                c2manufact_req;
    CAPI2_manufact_conf_t               c2manufact_conf;
    CAPI2_manufact_ind_t                c2manufact_ind;
    CAPI2_manufact_resp_t               c2manufact_resp;
    CAPI2_selectb_req_t                 c2selectb_req;
    CAPI2_selectb_conf_t                c2selectb_conf;

    /* 
     * this messages are only used for communication 
     * with BIANCA CAPI on a BinTec ISDN adapter running
     * BOSS (Bintec Open Streams System)
     */
    CAPI_control_req_t			control_req;
    CAPI_control_conf_t			control_conf;
    CAPI_control_conf_ex_t		control_conf_ex;
    CAPI_control_ind_t			control_ind;
    CAPI_control_resp_t			control_resp;

    CAPI_register_req_t			register_req;
    CAPI_register_conf_t		register_conf;
    CAPI_release_req_t			release_req;
    CAPI_release_conf_t			release_conf;
    CAPI_getmanufact_req_t		getmanufact_req;
    CAPI_getmanufact_conf_t		getmanufact_conf;
    CAPI_getserial_req_t		getserial_req;
    CAPI_getserial_conf_t		getserial_conf;
    CAPI_getversion_req_t		getversion_req;
    CAPI_getversion_conf_t		getversion_conf;
    CAPI_getprofile_req_t		getprofile_req;
    CAPI_getprofile_conf_t		getprofile_conf;

    CAPI_alive_ind_t			alive_ind;
    CAPI_alive_resp_t			alive_resp;
} CAPI_primitives_t;

typedef union {
    facreq_t	    req;
    dtmfreq_t       dtmf;
    ss_facreq_t     ssreq;
    ss_faccdreq_t   sscdreq;
    ss_facawsreq_t  ssawsreq;
}facreq_u;


#if !MAKROACCESS
# pragma pack()
#endif

#ifdef _cplusplus
}
#endif

#endif 	/* __CAPIDEF_H_ */
