/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: libcapi.h,v $
 *   $Revision: 56 $
 *       $Date: 2005-11-21 20:44:57 +0100 (Mon, 21 Nov 2005) $
 *      $State: Exp $
 *     
 *        Type: include file for ..
 *    Products: ALL | XS,XM,XL,XP,BGO,BGP,XCM,X1,X2,X3,X4,X8
 * Description: --
 *-----------------------------------------------------------------------
 * Current Log:    
 * 	- 
 ***********************************************************************/
#ifndef __LIBCAPI_H
#define __LIBCAPI_H

#include <capiconf.h>
#include <capidef.h>

#ifdef __MSDOS__
#   define FAR far
#else
#   define FAR
#endif

extern int capi_errno;
extern int capi2_errno;

#define CAPI_HOST_ENV		"CAPI_HOST"
#define CAPI_PORT_ENV		"CAPI_PORT"
#define CAPI_PORT_DEFAULT	2662

#ifndef MIN
#   define MIN(a,b)       ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#   define MAX(a,b)       ((a) > (b) ? (a) : (b))
#endif


/***********************************************************************
 *
 * structure to hold information about the remotcapihost
 * - first attemp to get somewhat of the flexibility of the remote
 *   CAPI usable without that stupid env
 *
 **********************************************************************/

typedef struct rcapi_host_s 
{
    char *hostname;
    char *service;
    int port;
} rcapi_host_t;

typedef struct rcapi_auth_s 
{
    char *user;
    char *passwd;
} rcapi_auth_t;


/*
 * CAPI 1.1 interface declarations
 */
int     capi_set_signal(int	appl,	/* XXX UNIMPLEMENTED XXX */
			void	*sig); 

int     capi_register(int		msgcnt,
		      int		level3cnt,
		      int		datablkcnt,
		      int		datablklen,
		      char  *		datablock);

int     capi_get_manufacturer(char *buffer);

int     capi_get_version(char *buffer);
int     capi_get_serial(char *buffer);
int     capi_release(int   	appl);
int     capi_put_message(int 		appl,
			 char		*a);
int     capi_get_message(int 			appl,
			 union CAPI_primitives **	cpp,
			 union CAPI_primitives *	cmsg,
			 char *			dbuf,
			 int			dlen);

/*
 * CAPI 1.1 library declarations
 */
unsigned short capi_listen_req(int 		appl,
			       unsigned char 	contrl,
			       unsigned long 	infomask,
			       unsigned short 	eazmask,
			       unsigned short 	simask);

unsigned short capi_connect_req(int 		appl,
				unsigned char 	contrl,
				unsigned char 	channel,
				unsigned long 	infomask,
				unsigned char 	service,
				unsigned char	addinfo,
				unsigned char	eaz,
				struct telno * dad_telno,
				struct telno * oad_telno);

unsigned short capi_getparams_req(int			appl,
				  unsigned short	plci);

unsigned short capi_connectinfo_req(int 		appl,
				    unsigned short 	plci,
				    struct telno *	telno);

unsigned short capi_connect_resp(int 		appl,
				 unsigned short messid,
				 unsigned short plci,
				 unsigned short reject);

unsigned short capi_selectb2_req(int 			appl,
				 unsigned short 	plci,
				 unsigned char  	proto,
				 struct userdata *	dlpd);

unsigned short capi_selectb3_req(int 			appl,
				 unsigned short 	plci,
				 unsigned char 		proto,
				 struct userdata *	ncpd);

unsigned short capi_connectactive_resp(int 		appl,
				       unsigned short 	messid,
				       unsigned short 	plci);

unsigned short capi_info_req(int 		appl,
			     unsigned short 	plci,
			     unsigned long 	infomask);

unsigned short capi_info_resp(int 		appl,
			      unsigned short 	messid,
			      unsigned short 	plci);

unsigned short capi_disconnect_req(int 			appl,
				   unsigned short 	plci,
				   unsigned char 	cause);

unsigned short capi_disconnect_resp(int 		appl,
				    unsigned short 	messid,
				    unsigned short 	plci);

unsigned short capi_data_req(int 		appl,
			     unsigned short 	plci,
			     struct userdata *	data);

unsigned short capi_data_resp(int 		appl,
			      unsigned short 	messid,
			      unsigned short 	plci);

unsigned short capi_listenb3_req(int 		appl,
				 unsigned short plci);

unsigned short capi_connectb3_req(int 			appl,
				  unsigned short 	plci,
				  struct userdata *	ncpi);

unsigned short capi_connectb3_resp(int 			appl,
				   unsigned short 	messid,
				   unsigned short 	ncci,
				   unsigned short 	reject,
				   struct userdata *	ncpi);

unsigned short capi_connectb3active_resp(int 		 appl,
					 unsigned short  messid,
					 unsigned short  ncci);

unsigned short capi_resetb3_req(int 		appl,
				unsigned short 	ncci);

unsigned short capi_resetb3_resp(int 		appl,
				 unsigned short messid,
				 unsigned short ncci);

unsigned short capi_disconnectb3_req(int 		appl,
				     unsigned short 	ncci,
				     struct userdata *	ncpi);

unsigned short capi_disconnectb3_resp(int 	      	appl,
				      unsigned short 	messid,
				      unsigned short 	ncci);
unsigned short capi_datab3_req(int 		appl,
			       unsigned short 	ncci,
			       char  *		buffer,
			       unsigned short  	len,
			       unsigned short	flags,
			       unsigned char	blknum);

unsigned short capi_datab3_resp(int 		appl,
				unsigned short  messid,
				unsigned short  ncci,
				unsigned char   blknum);

unsigned short capi_handset_resp(int 		appl,
				 unsigned short messid,
				 unsigned short plci);

unsigned short capi_dtmf_req(int 		appl,
			     unsigned short 	plci,
			     struct userdata *	data);

unsigned short capi_dtmf_resp(int 		appl,
			      unsigned short 	messid,
			      unsigned short 	plci);

unsigned short capi_control_req(int		appl,
				int		contrl,
				int		type,
				struct userdata *data);

unsigned short capi_control_resp(int			appl,
				 int			contrl,
				 int			messid,
				 int			type,
				 struct userdata	*data);


/*
 * CAPI 2.0 interface declarations (unix streams and remote capi)
 */
int             capi_open(void);
int             capi_close(int fd);
const char *    capi_msg(union CAPI_primitives *a);

void            capi_perror(const char 	*str, 
			    int 	info);

void            capi2_perror(const char *str, 
			     int	info);

int             capi2_open(void);
int             capi2_close(int fd);


/*
 * CAPI 2.0 interface declarations
 */
int capi2_set_signal(int	appl,	/* XXX UNIMPLEMENTED XXX */ 
		     void	*sig);	

int capi2_register(int		msgsize,
		   int		level3cnt,
		   int		datablkcnt,
		   int		datablklen,
		   char		*datablock);

int capi2_get_manufacturer(char *buffer);

int capi2_get_version(unsigned long *lPtrVersion );

int capi2_get_serial(char *buffer);

int capi2_get_profile(int			nCtrl, 
		      struct capi_getprofile	*prof);

int capi2_release(int	fd);

int capi2_put_message(int 	appl, 
		      char	 *a);

int capi2_get_message(int 			appl,
		      union CAPI_primitives **	cpp,
		      union CAPI_primitives *  	cmsg,
		      char *		       	dbuf,
		      int		       	dlen);

int capi2_wait_for_signal(int	 appl, 	
			  int	 timeout);


/*
 * CAPI 2.0 library declarations
 */
unsigned short capi2_listen_req(int 		appl,
				unsigned long 	contrl,
				unsigned long 	info_mask,
				unsigned long	cip_mask,
				unsigned long	cip_mask2,
				struct userdata *cpn,
				struct userdata *cps);

unsigned short capi2_alert_req(int		appl,
			       unsigned long	plci,
			       struct userdata	*add);

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
				 struct userdata 	*add);

unsigned short capi2_connect_resp(int			appl,
				  unsigned short	messid,
				  unsigned long		plci,
				  int			reject,
				  struct userdata 	*bprot, 
				  struct userdata	*cad, 
				  struct userdata	*csa, 
				  struct userdata	*llc, 
				  struct userdata	*add);

unsigned short capi2_connectactive_resp(int 		appl,
					unsigned short 	messid,
					unsigned long 	plci);

unsigned short capi2_info_req(int 		appl,
			      unsigned long 	ident,
			      struct userdata *cpn,
			      struct userdata *add);

unsigned short capi2_info_resp(int 		appl,
			       unsigned short 	messid,
			       unsigned long 	ident);

unsigned short capi2_disconnect_req(int 		appl,
				    unsigned long 	plci,
				    struct userdata 	*add);

unsigned short capi2_disconnect_resp(int 		appl,
				     unsigned short 	messid,
				     unsigned long 	plci);

unsigned short capi2_selectb_req(int 			appl,
				 unsigned long 		plci,
				 struct userdata *	bproto);

unsigned short capi2_facility_req(int 			appl,
				  unsigned long 	ident,
				  unsigned short	selector,
				  struct userdata *	facdata);

unsigned short capi2_facility_resp(int 			appl,
				   unsigned short	messid,
				   unsigned long 	ident,
				   unsigned short	selector,
				   struct userdata *	facdata);

unsigned short capi2_connectb3_req(int 			appl,
				   unsigned long 	plci,
				   struct userdata *	ncpi);

unsigned short capi2_connectb3_resp(int 		appl,
				    unsigned short 	messid,
				    unsigned long 	ncci,
				    unsigned short	reject,
				    struct userdata *	ncpi);

unsigned short capi2_connectb3active_resp(int 			appl,
					  unsigned short  	messid,
					  unsigned long   	ncci);

unsigned short capi2_connectb3t90active_resp(int 		 appl,
					     unsigned short   messid,
					     unsigned long   ncci);

unsigned short capi2_disconnectb3_req(int 		appl,
				      unsigned long 	ncci,
				      struct userdata *	ncpi);

unsigned short capi2_disconnectb3_resp(int 	      	appl,
				       unsigned short 	messid,
				       unsigned long 	ncci);

unsigned short capi2_resetb3_req(int 			appl,
				 unsigned long		ncci,
				 struct userdata	*ncpi);

unsigned short capi2_resetb3_resp(int 			appl,
				  unsigned short 	messid,
				  unsigned long   	ncci);

unsigned short capi2_datab3_req(int 			appl,
				unsigned long 		ncci,
				char FAR *		buffer,
				unsigned short  	len,
				unsigned short		flags,
				unsigned short		handle);

unsigned short capi2_datab3_resp(int 		appl,
				 unsigned short messid,
				 unsigned long  ncci,
				 unsigned short handle);

/***********************************************************************
 *	additional BinTec CAPI library declarations
 **********************************************************************/

int capi_read_environ(char	**ppHost,
		      int	*pPort);

int capi_tcp_open(char    *host,
		  char    *service,
		  int     port);

size_t capi_blockread(int 	fd, 
		      VOIDPTR 	block, 
		      size_t 	size );

size_t capi_blockwrite(int 	fd, 
		      VOIDPTR 	block, 
		      size_t 	size );

int capi2_checkuser( int fd );

int capi2_checkuser2(int	fd, 
		     char	*user, 
		     char	*passwd);


int capi_hexdump(char	*buf,
		 size_t	len,
		 size_t	lineWidth,
		 size_t	lineOffset);

int capi_hexdump_d(char		*buf,
		   size_t	len,
		   size_t	lineWidth,
		   size_t	lineOffset,
		   size_t	descLen);

int capi_fhexdump(FILE		*fp,
		  char		*buf,
		  size_t	len,
		  size_t	lineWidth,
		  size_t	lineOffset);

int capi_fhexdump_d(FILE	*fp,
		    char	*buf,
		    size_t	len,
		    size_t	lineWidth,
		    size_t	lineOffset,
		    size_t	descLen);

/***********************************************************************
 *
 * no comment
 *
 **********************************************************************/

int rcapi_register(int		msgsize,
			int		level3cnt,
			int		datablkcnt,
			int		datablklen,
			char FAR 	*datablock, 
			rcapi_host_t	*host,
			rcapi_auth_t	*auth);

int rcapi_open(rcapi_host_t *host);

int rcapi_get_version(unsigned long 	*lPtrVersion,
	rcapi_host_t	*host);

int rcapi_get_manufacturer(char		*buffer, 
	rcapi_host_t	*host);

int rcapi_get_serial(char			*buffer,
	rcapi_host_t	*host);

int rcapi_get_profile(int				nCtrl, 
			   struct capi_getprofile	*prof,
			   rcapi_host_t		*host);

const char *capi_strerror(int	info );
const char *capi2_strerror(int	info);


/*********************************************************************/

#ifdef __MSDOS__
unsigned short capi_control_req(  int, int, int, struct userdata *);
unsigned short capi_control_resp( int, int, int, int, struct userdata *);
int capi_check( int );
int capi2_check( int );
#endif


#endif          /*  __LIBCAPI_H  */
