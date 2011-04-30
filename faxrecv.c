/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: faxrecv.c,v $
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

#undef  RECOVER_BRICK_BOOT
#define RECOVER_BRICK_BOOT

#define MAX_CMD_SIZE                    256
#define DEFAULT_TRACE_CMD               "bricktrace -hFA %d"
#define CMD_LINE_ARGS			"?bqTi:c:d:l:n:f:t:v:w:"
#define DEFAULT_VERBOSE_LEVEL		2

#define REG_CAPI_MSG_SIZE		1024
#define REG_CAPI_LEVEL3CNT		1
#define REG_CAPI_DATA_BLK_SIZE		1024	/* compat with old BRICKS */
#define REG_WINDOW_SIZE			7

#define CHILD_KILL_DELAY		30	/* seconds      */
#define REGISTER_FAIL_TIMER		30	/* seconds      */
#define DEFAULT_KEEP_ALIVE_TIMER	10	/* seconds	*/


#define NEWSTATE( ptrCall, newstate) (ptrCall)->state = newstate;
#define CAPI_EVENT(event) { if (cfg.verbose >2) printf("%s: capi_%08lx: %s\n", name, ident, event );}
#define CAPI_EVENT2(l, event) { if (cfg.verbose >(l)) printf("%s: capi_%08lx: %s\n", name, ident, event );}
#define LISTEN_CIP_MASK ( 1<<1 | 1<<4 | 1<<17 )

/**********************************
 *
 **********************************/
CONST char	*name		= NULL;
CONST char	*prog_logo	= "BIANCA/CAPI 2.0 Fax Receive Server";

enum state_e {
	Disconnected,
	OverlapReceiving,
	D_ConnectPending,
	D_Connected,
	B_ConnectPending,
	Connected,
	B_DisconnectPending,
	D_DisconnectPending,
	IgnoreDisconnect,
};

/**********************************
 *	call specific data
 **********************************/
typedef struct call_s call_t;
struct call_s {
    call_t	 	*next;
    FILE 		*recvfp;		/* file of the received fax */
    enum state_e	state;
    unsigned long 	ident;			/* controller, plci, ncci */

    time_t		starttime;		/* time() of faxstart   */
    time_t		endtime;		/* time() of faxend	*/
    int 		ncnt; 			/* for overlap receiving */
    userdata_t	 	CalledPartyNumber;	/* for overlap receiving */
    userdata_t	 	CalledPartySubaddress;	/* for overlap receiving */
};

/**********************************   
 *  global configuration data
 **********************************/
struct {
    call_t		*calls;
    int 		ncnt; 			/* for overlap receiving */
    userdata_t	 	CalledPartyNumber;	/* bind address 	*/
    userdata_t	 	CalledPartySubaddress;	/* bind address 	*/
    userdata_t		Bprotocol; 	   /* protocol info for connect_resp */
} global = {
    NULL,	
    0,
    {0, {0}},
    {0, {0}},
    {0, {0}}
};

/**********************************   
 *  configuration data
 **********************************/
struct {
    CONST char		*szReceiveDir;		/* directory for received fax */
    unsigned char 	byCapiContr;		/* controller 		      */
    int 		verbose;		/* verbose level 	      */
    int			winsize;
    int         	b_channel;
    char       		*pcmd;
    char        	cmd[MAX_CMD_SIZE];
    int         	pid;
    int			fd;
    int			cipmask;
    int			call_accepted;
    int			fork_flag;
    CONST char		*stationid;
     char		*loctelno;
    CONST char		*logfile;
    char                *user;
    char                *passwd;
} cfg = {
    "/tmp/",					/* dir for received fax     */
    1,						/* default controller 	    */
    DEFAULT_VERBOSE_LEVEL,			/* verbose level            */
    REG_WINDOW_SIZE,				/* winsize                  */
    -1,                                     	/*  b-channel number        */
    NULL,                                   	/*  ptr to cmd string       */
    DEFAULT_TRACE_CMD,                      	/*  default trace cmd       */
    0,                                      	/*  store pid of cmd        */
    0,					    	/*  store fd from register  */
    LISTEN_CIP_MASK,			    	/*  current mask for listen */
    0,					    	/*  call_accepted flag      */
    0,
    "+49 911 87654321",				/* stationid		    */
    NULL,					/* loctelno		    */
    NULL,					/* logfile		    */
    NULL,					/* user		            */
    ""					        /* passwd		    */    
};
/*******************************************************************
 * local proto's
 *******************************************************************/
VOID catch_signal       PROTO((int signo));
int init_capi		PROTO((VOID));
int my_kill		PROTO((VOID));
int start_external	PROTO((VOID));
userdata_t	 *get_struct	PROTO((union CAPI_primitives *msg, char *ptr));
userdata_t	 *skip_struct	PROTO((union CAPI_primitives *msg, char *ptr));
VOID enqueue_call	PROTO((call_t *ptrCall));
int dequeue_call	PROTO((call_t *ptrCall));
call_t *alloc_call	PROTO((VOID));
VOID free_call		PROTO((call_t *ptrCall));
call_t *getCallbyPlci	PROTO((unsigned long plci));
call_t *getCallbyNcci	PROTO((unsigned long ncci));
call_t *getCallbyIdent	PROTO((unsigned long ident));
userdata_t	 *setCalledPartyNumber	PROTO((char *szCalledPartyNumber));
userdata_t	 *setCalledPartySubaddress	PROTO((char *szCalledPartySubaddress));
userdata_t	 *setBprotocolFAX	PROTO((int lHighResolution, int nFormat, CONST char *szStationID));
int getCapiInfo		PROTO((union CAPI_primitives *capi));
VOID doDisconnect	PROTO((int fd, call_t *ptrCall));
int matchBindings	PROTO((userdata_t	 *incCalledPNumber, userdata_t	 *incCalledPSubaddress));
VOID printAddress	PROTO((userdata_t	 *incCallingPNumber, userdata_t	 *incCallingPSubaddress));
call_t *createNewCallPtr	PROTO((unsigned long ident, userdata_t	 *incCalledPNumber, userdata_t	 *incCalledPSubaddress));
enum state_e handleConnectInd	PROTO((call_t **pptrCall, unsigned long ident, userdata_t	 *incCalledPNumber, userdata_t	 *incCalledPSubaddress, userdata_t	 *incCallingPNumber, userdata_t	 *incCallingPSubaddress));
int handleConnectB3ActiveInd	PROTO((call_t *ptrCall));
int handleDataB3Ind	PROTO((call_t *ptrCall, unsigned short messid, unsigned char *dataptr, size_t datalen, unsigned short handle, unsigned short flags));
VOID handleInfoInd	PROTO((call_t *ptrCall, unsigned short infonumber, userdata_t	 *data));
VOID handleDisconnectB3Ind	PROTO((call_t *ptrCall, unsigned short nReason, userdata_t	 *ncpi));
VOID handleDisconnectInd	PROTO((call_t *ptrCall, unsigned short nReason));
int capi_event		PROTO((int fd));
static VOID usage	PROTO((VOID));
int listen		PROTO((int fd, int mask));
int init_program	PROTO((int argc, char **argv));
int main		PROTO((int argc, char **argv));

#ifdef __STDC__
void my_exit      ( void );
#else
VOID my_exit      PROTO(( int *func));
#endif

/*******************************************************************
 *
 *******************************************************************/
int init_capi(VOID)
{
    int  fd = -1;
    /*
     *  open capi device
     */
    for(;;) {
	if(cfg.verbose > 2){
	    putchar('R');
	    fflush(stdout);
	}
	
        fd = capi2_register( REG_CAPI_MSG_SIZE,
				 REG_CAPI_LEVEL3CNT,
				 cfg.winsize,
				 REG_CAPI_DATA_BLK_SIZE,
				 NULL);

	if(fd >0 ) break;

	fprintf( stderr, "capi2_register failed! retry in %d seconds\n", 
		REGISTER_FAIL_TIMER	);
	sleep(REGISTER_FAIL_TIMER);
    }

    /*
     *  Capi, give me the right incoming calls!!
     */
    if(cfg.verbose > 2){
	putchar('L');
	fflush(stdout);
    }
    listen(fd, cfg.cipmask);

    return fd;
}
/*******************************************************************
 *
 *******************************************************************/
int my_kill(VOID)
{
    if(cfg.pid){
	/*
	 * if we have started a external program
	 * we signal that we are terminate
	 */
	fprintf( stderr,"%s: send signal SIGTERM to process.pid = %d\n",
		name, cfg.pid);
	sleep(CHILD_KILL_DELAY);	/* wait for silence an trace channel */
	kill(-cfg.pid, SIGTERM);
	cfg.pid = 0;
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
VOID catch_signal( signo )
    int signo;
{
    int	ret;

    if(signo!=SIGALRM) {
	if(signo){
	    fprintf( stderr, "\n%s: signo=(%d)\n", name, signo);
	}
	my_kill();
	exit ( -1 );
    }
    /*
     * SIGALRM arrived
     */
    if(cfg.verbose > 4){
	putchar('A');
	fflush(stdout);
    }
    ret = listen(cfg.fd, cfg.cipmask);
    if (ret){
	printf("capi_listen return %d\n", ret);
	capi2_release(cfg.fd);
	cfg.fd = init_capi();
    }
    signal(SIGALRM,	catch_signal);
    alarm(DEFAULT_KEEP_ALIVE_TIMER);   /* check is brick still alive ??? */
    return;
}
/*******************************************************************
 *
 *******************************************************************/
#ifdef __STDC__
void  my_exit( void  )
#else
VOID my_exit( func )
    int	       *func;
#endif
{
    my_kill();
}

/*******************************************************************
 * 
 *******************************************************************/
int start_external(VOID)
{
    char        buf[MAX_CMD_SIZE+20];
    char       *pbuf = buf;

    if( !cfg.pcmd || (strlen(cfg.pcmd) >= MAX_CMD_SIZE) ){
	fprintf( stderr, "no cmd or cmd too long\n");
	exit(1);
    }
    /* build cmd line and substitute %d for current bchannel */
    pbuf += strlen(strcat(pbuf, "exec "));
    sprintf(pbuf, cfg.pcmd,
	    cfg.b_channel,
	    cfg.b_channel,
	    cfg.b_channel,
	    cfg.b_channel,
	    cfg.b_channel );

    fprintf( stderr, "%s: start external program: '%s'\n", name, buf );
    cfg.pid = fork();
    if(cfg.pid == 0){
	setpgid(0, 0);
	execlp("sh", "sh", "-c", buf, NULL);
    }
    fprintf( stderr, "%s: program pid = %d\n", name, cfg.pid);
    return(cfg.pid);
}
 

/*******************************************************************
 *	utilities for scanning capi structs
 *******************************************************************/
userdata_t	 *get_struct( msg, ptr)
union CAPI_primitives *msg;
char *ptr;
{
    char *end;
    end = (char *)msg + GET_LEN(msg);
 
    if (ptr > end) return NULL;
    return (userdata_t	 *)ptr;
}

/*******************************************************************
 *
 *******************************************************************/
userdata_t	 *skip_struct( msg, ptr)
union CAPI_primitives *msg;
char *ptr;
{
    char *end;
    userdata_t	 *data = (userdata_t	 *)ptr;
 
    end = (char *)msg + GET_LEN(msg);
 
    data = (userdata_t	 *)&data->data[data->length];
    if ((char *)data > end) return NULL;
    return data;
}

/*******************************************************************
 *	utilities for call pointer (ptrCall) manipulation
 *******************************************************************/
VOID enqueue_call( ptrCall )
call_t *ptrCall;
{
    ptrCall->next = global.calls;
    global.calls  = ptrCall;
}

/*******************************************************************
 *
 *******************************************************************/
int dequeue_call( ptrCall )
call_t *ptrCall;
{
    call_t **pp;

    for (pp=&(global.calls); *pp; pp=&(*pp)->next) {
	if (*pp == ptrCall) {
	    *pp = ptrCall->next;
	    return 1;
	}
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *alloc_call()
{
    call_t *ptrCall;

    ptrCall = (call_t *)malloc( sizeof(*ptrCall));

    if (!ptrCall) return NULL;
    memset( ptrCall, 0, sizeof(*ptrCall));
    return ptrCall;
}

/*******************************************************************
 *
 *******************************************************************/
VOID free_call( ptrCall )
call_t *ptrCall;
{
    if (ptrCall) {
	dequeue_call( ptrCall);
	if (ptrCall->recvfp) {
	    fclose( ptrCall->recvfp);
	}
	free(ptrCall);
    }
}

/*******************************************************************
 *	find the right call pointer for an identifier
 *******************************************************************/
call_t *getCallbyPlci( plci )
unsigned long plci;
{
    call_t *ptrCall;

    plci &= 0xffff;
    for(ptrCall = global.calls; ptrCall; ptrCall = ptrCall->next) {
	if ((ptrCall->ident & 0xffff) == plci) return ptrCall;
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyNcci( ncci )
unsigned long ncci;
{
    call_t *ptrCall;

    for(ptrCall = global.calls; ptrCall; ptrCall = ptrCall->next) {
	if (ptrCall->ident == ncci) return ptrCall;
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyIdent( ident )
unsigned long ident;
{
    if (ident >= 0xffff) 		return getCallbyNcci( ident );
    else if (ident >= 0xff) 		return getCallbyPlci( ident );
    else 				return NULL;
}

/*******************************************************************
 *	initialize global struct CalledPartyNumber
 *******************************************************************/
userdata_t	 *setCalledPartyNumber( szCalledPartyNumber )
char *szCalledPartyNumber;
{
    size_t len;

    if (szCalledPartyNumber) {
	len = strlen( szCalledPartyNumber);
	global.CalledPartyNumber.length  = len + 1;
	global.CalledPartyNumber.data[0] = 0x80;

	memcpy( &global.CalledPartyNumber.data[1], 
		szCalledPartyNumber, 
		len);
    }
    else {
	global.CalledPartyNumber.length = 0;
    }
    return &global.CalledPartyNumber;
}


/*******************************************************************
 *	initialize global struct CalledPartySubaddress
 *******************************************************************/
userdata_t	 *setCalledPartySubaddress( szCalledPartySubaddress)
char *szCalledPartySubaddress;
{
    size_t len;

    if (szCalledPartySubaddress) {
	len = strlen(szCalledPartySubaddress);
	global.CalledPartySubaddress.length  = len + 1;
	global.CalledPartySubaddress.data[0] = 0x80;

	memcpy( &global.CalledPartySubaddress.data[1],
		szCalledPartySubaddress,
		len);

    }
    else {
	global.CalledPartySubaddress.length = 0;
    }
    return &global.CalledPartySubaddress;
}


/*******************************************************************
 *	initialize global struct Bprotocol (for connect_resp )
 *******************************************************************/
userdata_t	 *setBprotocolFAX( lHighResolution, nFormat, szStationID)
int  lHighResolution;
int  nFormat;
CONST char *szStationID;
{
    struct bprotocol 	  *bprot;
    userdata_t	 	  *data;
    struct b3config_faxg3 *b3cfg;

    bprot = (struct bprotocol *)&global.Bprotocol;

    PUT_WORD( bprot->b1proto, B1FAXG3);
    PUT_WORD( bprot->b2proto, B2T30);
    PUT_WORD( bprot->b3proto, B3T30);

    data = (userdata_t	 *)&bprot->structlen;
    data->length = 0;	/*  b1config  */
    data = (userdata_t	 *)&data->data[data->length];
    data->length = 0;	/*  b2config  */
    data = (userdata_t	 *)&data->data[data->length];

    b3cfg = (struct b3config_faxg3 *)data;

    PUT_WORD( b3cfg->resolution, lHighResolution ? 1 : 0);
    PUT_WORD( b3cfg->format,     nFormat);	

    data = (userdata_t	 *)&b3cfg->structlen;
    data->length = strlen(szStationID);
    memcpy( data->data, szStationID, data->length);

    data = (userdata_t	 *)&data->data[data->length];
    data->length = 0; 	/* Headline */

    data = (userdata_t	 *)&data->data[data->length];

    b3cfg->length = (char *)data - (char *)&b3cfg->length - 1;
    bprot->length = (char *)data - (char *)&bprot->length - 1;

    return (userdata_t	 *)bprot;
}

/*******************************************************************
 *
 *******************************************************************/
int getCapiInfo( capi )
union CAPI_primitives *capi;
{
    int info;

    switch (GET_PRIMTYPE(capi)) {
	case CAPI2_LISTEN_CONF:
	    info = GET_WORD(capi->c2listen_conf.info);
	    break;
	case CAPI2_ALERT_CONF:
	    info = GET_WORD(capi->c2alert_conf.info);
	    break;
	case CAPI2_CONNECT_CONF:		
	    info = GET_WORD(capi->c2connect_conf.info);
	    break;
	case CAPI2_INFO_CONF:
	    info = GET_WORD(capi->c2info_conf.info);
	    break;
	case CAPI2_CONNECTB3_CONF:
	    info = GET_WORD(capi->c2connectb3_conf.info);
	    break;
	case CAPI2_DATAB3_CONF:
	    info = GET_WORD(capi->c2datab3_conf.info);
	    break;
	case CAPI2_RESETB3_CONF:
	    info = GET_WORD(capi->c2resetb3_conf.info);
	    break;
	case CAPI2_DISCONNECTB3_CONF:
	    info = GET_WORD(capi->c2disconnectb3_conf.info);
	    break;
	case CAPI2_DISCONNECT_CONF:
	    info = GET_WORD(capi->c2disconnect_conf.info);
	    break;
	case CAPI2_FACILITY_CONF:
	    info = GET_WORD(capi->c2facility_conf.info);
	    break;
	case CAPI2_SELECTB_CONF:
	    info = GET_WORD(capi->c2selectb_conf.info);
	    break;
	default:
	    info = 0;
	    break;
    }
    return (info);
}


/*******************************************************************
 *
 *******************************************************************/
VOID doDisconnect(fd, ptrCall)
int fd;
call_t *ptrCall;
{
    unsigned long plci, ncci;

    ncci = ptrCall->ident;
    plci = ptrCall->ident & 0xffff;

    switch (ptrCall->state) {
	case B_ConnectPending:
	case Connected:
	    NEWSTATE( ptrCall, B_DisconnectPending);
	    capi2_disconnectb3_req( fd, ncci, NULL);
	    break;

	case D_ConnectPending:
	case B_DisconnectPending:
	    NEWSTATE( ptrCall, D_DisconnectPending);
	    capi2_disconnect_req( fd, plci, NULL);

	    break;

	case D_DisconnectPending:
	case Disconnected:
	default:
	    break;
    }
}

/*******************************************************************
 *
 *******************************************************************/
int matchBindings(incCalledPNumber, incCalledPSubaddress)
userdata_t	 	* incCalledPNumber;
userdata_t	 	* incCalledPSubaddress;
{
    int i;
    unsigned char 	* baddr, *caddr;

    /* 
     *  check ISDN-address 
     *  the incoming address may be longer then the bind address 
     */
    i = global.CalledPartyNumber.length; 
    if (!i) return 1;
    i--;
    if ( i > incCalledPNumber->length) {
	return 0;
    }

    baddr = global.CalledPartyNumber.data + global.CalledPartyNumber.length;
    caddr = incCalledPNumber->data + incCalledPNumber->length;

    while (i > 0) {
	i--;
	if (*(--caddr) != *(--baddr)) {
	    return 0;
	}
    }

    /* 
     *  check ISDN-subaddress 
     *  incoming subaddress and bind subaddress must have the same
     *  length and the same digits
     */
    i = global.CalledPartySubaddress.length; 
    if ( i != incCalledPSubaddress->length) {
	return 0;
    }

    baddr = global.CalledPartySubaddress.data + global.CalledPartySubaddress.length;
    caddr = incCalledPSubaddress->data + incCalledPSubaddress->length;

    while (i > 0) {
	i--;
	if (*(--caddr) != *(--baddr)) {
	    return 0;
	}
    }
    return 1;
}

/*******************************************************************
 *
 *******************************************************************/
VOID printAddress(incCallingPNumber, incCallingPSubaddress)
userdata_t	         * incCallingPNumber;
userdata_t	         * incCallingPSubaddress;
{
    /* 
     * print the Calling Party Number of an incoming call
     */
    if (cfg.verbose > 1) {
	if (incCallingPNumber && incCallingPNumber->length >2) 
	    printf("Incoming call from : '%*.*s'\n", 
			incCallingPNumber->length -2,
			incCallingPNumber->length -2,
			incCallingPNumber->data +2);
	if (incCallingPSubaddress && incCallingPSubaddress->length >1) 
	    printf("\t Subaddress: '%*.*s'\n", 
			incCallingPSubaddress->length -1,
			incCallingPSubaddress->length -1,
			incCallingPSubaddress->data +1);
    }
}

/*******************************************************************
 *
 *******************************************************************/
call_t * createNewCallPtr(ident, incCalledPNumber, incCalledPSubaddress)
unsigned long ident;
userdata_t	         * incCalledPNumber;
userdata_t	         * incCalledPSubaddress;
{
    call_t * ptrCall;

    /* 
     *  allocate new call pointer 
     */
    ptrCall = alloc_call();
    if (!ptrCall) {
	fprintf( stderr, "alloc_call failed!\n");
	return NULL;
    }
    enqueue_call( ptrCall );

    /* 
     *  set the values
     */
    ptrCall->ident = ident;

    if (incCalledPNumber->length) 
	memcpy ((char *) &ptrCall->CalledPartyNumber, 
		(char *) incCalledPNumber, 
		(size_t)incCalledPNumber->length +1);
    else
	ptrCall->CalledPartyNumber.length = 0;

    if (incCalledPSubaddress->length) 
	memcpy ((char *) &ptrCall->CalledPartySubaddress, 
		(char *) incCalledPSubaddress, 
		(size_t)incCalledPSubaddress->length +1);
    else
	ptrCall->CalledPartySubaddress.length = 0;

    return ptrCall;
}

/*******************************************************************
 *
 *******************************************************************/
enum state_e handleConnectInd(pptrCall, ident, incCalledPNumber, incCalledPSubaddress, incCallingPNumber, incCallingPSubaddress)
    call_t		**pptrCall;
    unsigned long	ident;
    userdata_t	 	*incCalledPNumber;
    userdata_t	 	*incCalledPSubaddress;
    userdata_t	 	*incCallingPNumber;
    userdata_t	 	*incCallingPSubaddress;
{
    if ( global.ncnt) {
	/*
	 *  we have to wait for ncnt digits before we can check the bindings
	 */
	*pptrCall = createNewCallPtr(ident, 
				     incCalledPNumber, 
				     incCalledPSubaddress);
	if (!*pptrCall)
		return Disconnected;
	printAddress(incCallingPNumber, incCallingPSubaddress);
	return OverlapReceiving;
    }

    if ( matchBindings(incCalledPNumber, incCalledPSubaddress)) {
	/*
	 *  Address complete - it's the right number
	 */
	*pptrCall = createNewCallPtr(ident, 
				     incCalledPNumber, 
				     incCalledPSubaddress);
	if (!*pptrCall)
		return Disconnected;
	printAddress(incCallingPNumber, incCallingPSubaddress);
	return D_ConnectPending;
    }
    else {
	/*
	 *  Address complete - it's a wrong number
	 */
	if (cfg.verbose > 2) {
	    printf("Called Party number:\n");
	    capi_hexdump( incCalledPNumber->data,
		    incCalledPNumber->length, 16, 0); 
	    printf("doesn't match own number:\n");
	    capi_hexdump( global.CalledPartyNumber.data,
		    global.CalledPartyNumber.length, 16, 0); 
	}
	return IgnoreDisconnect;
    }
}

/*******************************************************************
 *
 *******************************************************************/
int handleConnectB3ActiveInd( ptrCall )
call_t *ptrCall;
{
    char	filename[255];
    struct stat	buf;
    static int	cnt = 0;
    int		ret;
    int		pathlen;

    ptrCall->starttime               = time(0);

    /* 
     *  create a new filename 
     */
    strcpy(filename, cfg.szReceiveDir);
    pathlen = strlen(filename);
    if (pathlen >0 && filename[pathlen-1] != '/') {
	filename[pathlen++] = '/';
    }

    do {
	sprintf(filename +pathlen, "fax%04d.sff", cnt);
	ret = stat(filename, &buf);
	cnt++;
    } while (ret == 0); 	/* file exists ? */

    if (ret < 0 && errno != ENOENT ) {
	perror(filename); 
	return 0;
    }

    /* 
     *  create and open the file to store the data
     */
    ptrCall->recvfp = fopen(filename, "w");
    if (!ptrCall->recvfp) {
	perror(filename);
	return 0;
    }
    if (cfg.verbose > 1) {
	printf("Storing incoming Fax data in file %s \n", filename);
	printf("Receiving Fax data: ");
    }
    return 1;
}


/*******************************************************************
 *
 *******************************************************************/
int handleDataB3Ind( ptrCall, messid, dataptr, datalen, handle, flags)
call_t		*ptrCall;
unsigned short	messid;
unsigned char	*dataptr;
size_t		datalen;
unsigned short	handle;
unsigned short	flags;
{
    int		len;

    if (cfg.verbose > 1) {
	putchar('r');
	fflush(stdout);
    }

    /*
     *  store the incoming data in the file ptrCall->recvfp
     */
#if 1
    len = fwrite(dataptr, 1, datalen, ptrCall->recvfp);
    if ( len <= 0) {
	if(feof(ptrCall->recvfp)) {
	    perror("fwrite:EOF file system full ???");
	    printf("fwrite returns = %d\n", len);
	    return 0;
	}
	if( len < 0 ) {
	    perror("fwrite:ERR ???");
	    return 0;
	}
	perror("fwrite:EOF ??? ");
	return 0;
    }	
#else
    for (len=0; len<datalen; len++) {
	if (putc(((char*)dataptr)[len], ptrCall->recvfp) == EOF)  {
	    printf("putcerror\n");
	    return 0;
	}

    }
#endif
    return 1;	/*  response */
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleInfoInd( ptrCall, infonumber, data)
    call_t		*ptrCall;
    unsigned short	infonumber;
    userdata_t		*data;
{
    int 		i = 0;
    unsigned long       mask;
    unsigned char 	*datap = data->data;

    switch(infonumber){
	default:
	    if (cfg.verbose > 5) {
		printf("\tInfoInd 0x%04x\n", infonumber);
		capi_hexdump( data->data, data->length, 16, 2); 
	    }
	    break;
	case AI_DAD:
	    if (ptrCall->state == OverlapReceiving) {
		/* 
		 *  collect digits for the CalledPartyNumber
		 */
/*!!HB!! BUGBUG */
		while (i < data->length ) {
		    if ((data->data[i++] & 0x80 )) break;
		}

		memcpy( &ptrCall->CalledPartyNumber.data[ptrCall->CalledPartyNumber.length], 
			&data->data[i],
			(size_t)data->length -i);
		ptrCall->CalledPartyNumber.length += data->length -i;
		ptrCall->ncnt += data->length -i;

	    }
	    break;
	case 0x18:	/* IBCHI        */
	    if (*datap & (1<<5)) {
		/* primary rate interface */
		if (*datap++ & (1<<6)) {
		    /* interface identifier present */
		    while((*datap++ & 0x80) == 0);
		}
		if (*datap++ & (1<<4)) {
		    cfg.b_channel = 1;
		    mask = 1;
		    while(!(mask & *datap)){
			mask<<=1;
			cfg.b_channel++;
		    }
		}
		else {
		    cfg.b_channel = *datap & 0x7f;
		}
	    }
	    else {
		/* basic rate interface */
		cfg.b_channel = *datap & 0x03;
	    }
	    if(cfg.verbose>2){
		printf("\tINFOIND: ISDN-BCHANNEL %d\n", cfg.b_channel);
	    }
	    /* call the external program if there is a request */
	    if(cfg.pcmd){
		start_external();
		cfg.pcmd  = NULL;       /* do it only once */
	    }
	    break;
    }
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleDisconnectB3Ind( ptrCall, nReason, ncpi)
    call_t		*ptrCall;
    unsigned short	nReason;
    userdata_t		*ncpi;
{
    struct ncpi2_fax	*fax = (struct ncpi2_fax *)ncpi;
    int			delta_time;

    ptrCall->endtime = time(0);
    delta_time  = ptrCall->endtime - ptrCall->starttime;

    printf("\nFax T.30 Disconnect\n");
    if (nReason) printf("\tB3_Reason: (0x%04x)\n", nReason);

    if (fax->length >= sizeof(*fax)) {
	printf("\tRemote Station ID : '%*.*s'\n", fax->structlen,
		fax->structlen,
		&fax->structlen+1);
	printf("\tTransfer-Rate     : %d bps\n", GET_WORD(fax->rate));
	printf("\tResolution        : %s\n",	 GET_WORD(fax->resolution) ?
		"high (196 DPI)" : "low (98 DPI)");
	printf("\tNumber of Pages   : %d\n",     GET_WORD(fax->pages));
	printf("\tFormat            : %d\n",     GET_WORD(fax->format));
	printf("\tTransfer-Time     : %d:%d (min:sec)\n",
		delta_time / 60, delta_time % 60);
	printf("\n");
    }
    else {
	printf("\tWRONG LEN ???\n");
	goto handleDisconnectB3Ind_dump;
    }
    if (cfg.verbose > 5) {
handleDisconnectB3Ind_dump:
	printf("\tstruct ncpi2_fax length=%d\n", (size_t)fax->length);
	capi_hexdump((char*)fax, (size_t)fax->length, 0x10, 2);
	fflush(stdout);
    }
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleDisconnectInd( ptrCall, nReason)
    call_t *ptrCall;
    unsigned short nReason;
{
    if (cfg.verbose > 1) {
	printf("ISDN D-channel disconnect, Reason: 0x%04x\n", nReason);
    }
}


/*******************************************************************
 *
 *******************************************************************/
int capi_event(fd)
    int fd;
{
    union CAPI_primitives *capi;
    int			info;
    int			lcause;
    unsigned long	ident;
    unsigned char 	proto[REG_CAPI_MSG_SIZE];
    unsigned char  	data[4096];
    userdata_t	 	* incCalledPNumber;
    userdata_t	 	* incCalledPSubaddress;
    userdata_t	 	* incCallingPNumber;
    userdata_t	 	* incCallingPSubaddress;
    call_t		* ptrCall;
    enum state_e	newstate;

    lcause = capi2_get_message( fd, &capi, 
	    (union CAPI_primitives *) proto, 
	    data, sizeof(data));

    /* 
     *  check for errors
     */
    switch (lcause) {
	case CAPI2_E_MSG_QUEUE_EMPTY:	 	return  0;		
	case CAPI2_E_INTERNAL_BUSY_CONDITION:	return -1;
	default:				break;
    }

    info    = getCapiInfo(capi);
    if (info) capi2_perror("CAPI Info: ", info);

    /*
     *  get the call structure
     */
    ident   = GET_c2IDENT(capi);
    ptrCall = getCallbyIdent(ident);

    switch (GET_PRIMTYPE(capi)) {
	case CAPI2_CONNECT_IND:
	    CAPI_EVENT("CAPI2_CONNECT_IND");
fprintf( stderr, "CAPI2_CONNECT_IND cip_value=%d\n", 
			GET_WORD(capi->c2connect_ind.cip_value));
	    incCalledPNumber      = get_struct( capi, 
					&capi->c2connect_ind.structlen);
	    incCallingPNumber     = skip_struct( capi, 
					(char *)incCalledPNumber);
	    incCalledPSubaddress  = skip_struct( capi, 
					(char *)incCallingPNumber);
	    incCallingPSubaddress = skip_struct( capi, 
					(char *)incCalledPSubaddress);
	    newstate = handleConnectInd(&ptrCall, ident, 
					incCalledPNumber,
					incCalledPSubaddress,
					incCallingPNumber,
					incCallingPSubaddress);
	    switch (newstate) {
		case OverlapReceiving:
		    /*
		     *  wait for InfoInd with further numberdigits, send 
		     *  capi2_connect_resp when CalledPartyNumber is complete
		     */
		    break;
		case D_ConnectPending:
		    /* 
		     *  accept ConnectInd, wait for ConnectActiveInd
		     */
		    capi2_connect_resp( fd, 
			    (unsigned short)GET_MESSID(capi),
			    ident,        
			    0, 			/* accept */
			    &global.Bprotocol,
			    NULL,  			/* cad */
			    NULL,  			/* csa */
			    NULL,  			/* llc */
			    NULL); 			/* add */
		    cfg.call_accepted = 1;
		    break;
		case IgnoreDisconnect:
		    /*
		     * got wrong CalledPartyNumber or CalledPartySubaddress
		     */
		    capi2_connect_resp( fd, 
			    (unsigned short)GET_MESSID(capi),
			    ident, 
			    1, 			/* ignore */
			    &global.Bprotocol,
			    NULL,  			/* cad */
			    NULL,  			/* csa */
			    NULL,  			/* llc */
			    NULL); 			/* add */
		    break;
		case Disconnected:
		    /*
		     * something internal failed
		     */
		    capi2_connect_resp( fd, 
			    (unsigned short)GET_MESSID(capi),
			    ident, 
			    8, 		/* out of order */
			    &global.Bprotocol,
			    NULL,  			/* cad */
			    NULL,  			/* csa */
			    NULL,  			/* llc */
			    NULL); 			/* add */
		    break;
		default:
		    break;
	    }
	    if (ptrCall) {
		NEWSTATE( ptrCall, newstate);
	    }
	    break;
	
	case CAPI2_INFO_IND:
	    CAPI_EVENT("CAPI2_INFO_IND");
	    capi2_info_resp( fd, (unsigned short)GET_MESSID(capi), ident);
	    if (ptrCall) {
		handleInfoInd( ptrCall,
			GET_WORD(capi->c2info_ind.info),
			(userdata_t	 *)&capi->c2info_ind.structlen);
		if (ptrCall->state == OverlapReceiving &&
			ptrCall->ncnt >= global.ncnt) {

		    /*
		     *  CalledPartyNumber is complete now
		     */
		    if (matchBindings(&ptrCall->CalledPartyNumber, 
				      &ptrCall->CalledPartySubaddress)) {
			capi2_connect_resp( fd, 
				(unsigned short)GET_MESSID(capi),
				ident, 
				0, 			/* accept */
				&global.Bprotocol,
				NULL, 		/* cad */
				NULL, 		/* csa */
				NULL, 		/* llc */
				NULL); 		/* add */
			cfg.call_accepted = 1;
			NEWSTATE( ptrCall, D_ConnectPending);
		    }
		    else {
			if (cfg.verbose > 2) {
			    printf("Called Party number:\n");
			    capi_hexdump( ptrCall->CalledPartyNumber.data, 
				ptrCall->CalledPartyNumber.length, 16, 0); 
			    printf("doesn't match own number:\n");
			    capi_hexdump( global.CalledPartyNumber.data, 
				global.CalledPartyNumber.length, 16, 0); 
			}
			capi2_connect_resp( fd, 
				(unsigned short)GET_MESSID(capi),
				ident, 
				1, 			/* ignore */
				&global.Bprotocol,
				NULL, 		/* cad */
				NULL, 		/* csa */
				NULL, 		/* llc */
				NULL); 		/* add */
			NEWSTATE( ptrCall, IgnoreDisconnect);
		    }
		}
	    }
	    break;
	
	case CAPI2_CONNECTACTIVE_IND:
	    CAPI_EVENT("CAPI2_CONNECTACTIVE_IND");
	    capi2_connectactive_resp( fd,
		    (unsigned short)GET_MESSID(capi),
		    ident);
	    NEWSTATE( ptrCall, D_Connected);
	    /* 
	     *  now wait for the CONNECTB3_IND
	     */
	    break;
	    
	case CAPI2_CONNECTB3_IND:
	    CAPI_EVENT("CAPI2_CONNECTB3_IND");
	    ptrCall = getCallbyPlci( ident & 0xffff );
	    if (ptrCall) {
		/*
		 * NCCI allocation -> store NCCI for later use
		 */
		ptrCall->ident = ident;
		capi2_connectb3_resp( fd,
			(unsigned short)GET_MESSID(capi),
			ident,
			0,
			NULL);
		NEWSTATE( ptrCall, B_ConnectPending);
	    }
	    /* 
	     *  now wait for the CONNECTB3ACTIVE_IND
	     */
	    break;
	    
	case CAPI2_CONNECTB3ACTIVE_IND:
	    CAPI_EVENT("CAPI2_CONNECTB3ACTIVE_IND");
	    capi2_connectb3active_resp( fd,
		    (unsigned short)GET_MESSID(capi),
		    ident);
	    if (ptrCall) {
		NEWSTATE( ptrCall, Connected);
		/* 
		 *  open a file to store the incoming DATAB3_INDs
		 */
		if ( ! handleConnectB3ActiveInd(ptrCall)){
		    doDisconnect(fd, ptrCall );		/* file open failed */
		}
	    }
	    /* 
	     *  now wait for DATAB3_IND
	     */
	    break;

	case CAPI2_DATAB3_IND:
	    CAPI_EVENT2(5, "CAPI2_DATAB3_IND");
	    if (ptrCall) {
		if (handleDataB3Ind( ptrCall,
			     (unsigned short)GET_MESSID(capi),
			     (char *)GET_DWORD(capi->c2datab3_ind.dataptr),
			     GET_WORD( capi->c2datab3_ind.datalen),
			     GET_WORD( capi->c2datab3_ind.handle),
			     GET_WORD( capi->c2datab3_ind.flags))) {
		    capi2_datab3_resp( fd,
			    (unsigned short)GET_MESSID(capi),
			    ident,
			    GET_WORD(capi->c2datab3_ind.handle));
		}
	    }
	    break;

	case CAPI2_DISCONNECTB3_IND:
	    CAPI_EVENT("CAPI2_DISCONNECTB3_IND");
	    if (ptrCall) {
		NEWSTATE( ptrCall, B_DisconnectPending);
		handleDisconnectB3Ind( ptrCall, 
		    GET_WORD(capi->c2disconnectb3_ind.reason_b3),
		    (userdata_t	 *)&capi->c2disconnectb3_ind.structlen);
	    }	
	    capi2_disconnectb3_resp( fd,
		    (unsigned short)GET_MESSID(capi),
		    ident);

	    /*
	     * disconnect D-channel, perhaps your partner forgets it
	     */
	    if (ptrCall) NEWSTATE( ptrCall, D_DisconnectPending);
	    capi2_disconnect_req( fd, ident & 0xffff, NULL);
	    break;
		
	case CAPI2_DISCONNECT_IND:
	    CAPI_EVENT("CAPI2_DISCONNECT_IND");
	    capi2_disconnect_resp( fd,
		    (unsigned short)GET_MESSID(capi),
		    ident);

	    if (ptrCall) {
		NEWSTATE( ptrCall, Disconnected);
		handleDisconnectInd(ptrCall, 
			GET_WORD(capi->c2disconnect_ind.reason));
		if(ptrCall->state==IgnoreDisconnect){
		    free_call( ptrCall );
		    return 0;
		}
	    }
	    free_call( ptrCall );
	    return 1;
	    break;
	case CAPI2_LISTEN_CONF:
	    CAPI_EVENT2(5, "CAPI2_LISTEN_CONF");
	    break;
	case CAPI2_DISCONNECTB3_CONF:
	    CAPI_EVENT("CAPI2_DISCONNECTB3_CONF");
	    break;
	case CAPI2_DISCONNECT_CONF:
	    CAPI_EVENT("CAPI2_DISCONNECT_CONF");
	    break;
	case CAPI2_FACILITY_CONF:
	case CAPI2_ALERT_CONF:
	case CAPI2_INFO_CONF:
	default:
	    CAPI_EVENT("unexpectected CAPI event");
	    break;
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
static VOID usage(VOID)
{
    fprintf(stderr, "\n\tusage:\t%s [-%s]\n", name, CMD_LINE_ARGS);
    fprintf(stderr, "\tOptions:\n");
    fprintf(stderr, "\t-b: working in the background\n");
    fprintf(stderr, "\t-c: specify ISDN controller to use [1..n]\n");
    fprintf(stderr, "\t-d: specify default directory for faxfiles\n");
    fprintf(stderr, "\t-f: specify a logfile\n");

    fprintf(stderr, "\t-n: specify count of number digits to wait for\n");
    fprintf(stderr, "\t-l: specify local ISDN-address/subaddress\n");
    
    fprintf(stderr, "\t-i: specify local station identification\n");
    fprintf(stderr, "\t-q: use quiet mode\n");
    fprintf(stderr, "\t-v: change verbose level (default=%d)\n", DEFAULT_VERBOSE_LEVEL);
    fprintf( stderr, "\t-w: specify data window size (default=%d)\n",
	    REG_WINDOW_SIZE);
    fprintf(stderr, "\t-t: exec <cmd %%d> external program when bchannel established\n");
    fprintf(stderr, "\t-T: <exec %s> when bchannel established\n" ,
	    DEFAULT_TRACE_CMD );
    fprintf( stderr, "\texample:\n\t%s -t'bricktrace -Hbrick1 -hFA %%d 0 3' ...\n", name);
    fprintf( stderr, "\t%s -t'bricktrace -Hbrick1 -hFA %%d 0 3 >trace%%d.log' ...\n", name);


}
/*******************************************************************
 *
 *******************************************************************/
int listen( fd, mask )
    int fd;
    int mask;
{
    int ret;
    ret = capi2_listen_req( fd,
		      cfg.byCapiContr, 	/* Controller */
		      0x000001ff,	/* Info Mask */
		      mask,	 	/* CIP Mask */
		      0,		/* CIP Mask2 */
		      NULL, 		/* Calling party number */
		      NULL);		/* Calling party subaddress */

    return ret;
}

/*******************************************************************
 *
 *******************************************************************/
int init_program(argc, argv)
    int argc;
    char **argv;
{
    name = argv[0];

#ifdef __STDC__
    atexit( my_exit );
#else
    on_exit( my_exit, 0);
#endif
    signal(SIGHUP,      catch_signal);
    signal(SIGINT,      catch_signal);
    signal(SIGQUIT,     catch_signal);
    signal(SIGALRM,	catch_signal);
    signal(SIGTERM,     catch_signal);
    signal(SIGUSR1,     catch_signal);
    signal(SIGUSR2,     catch_signal);

    return 0;
}
/*******************************************************************
 *
 *******************************************************************/
int main(argc, argv)
int argc;
char **argv;
{
    int 		i;
    int 		endloop = 0;
    struct pollfd	pfd;
    struct capi_getprofile profile_str;

    extern char 	*optarg;
    extern int		capi2_errno;
          
    init_program(argc, argv);

    /* 
     * get options
     */
    while ((i = getopt(argc, argv, CMD_LINE_ARGS)) != EOF) {
	switch (i) {
	    case 'i':	cfg.stationid	= optarg;	break;
	    case 'l':	cfg.loctelno	= optarg;	break;
            case 'b':	cfg.fork_flag	= 1;		break;
            case 'c':	cfg.byCapiContr	= atoi(optarg);
		if (cfg.byCapiContr < 1 || cfg.byCapiContr > 255) {
		    printf("Invalid Controller - using Controller 1 \n");
		    cfg.byCapiContr = 1;
		}
                break;
            case 'd':	cfg.szReceiveDir= optarg;	break;
            case 'f':	cfg.logfile	= optarg;	break;
            case 'q':	cfg.verbose	= 0;		break;
            case 'v':	cfg.verbose	= atoi(optarg);	break;
	    case 'w':	cfg.winsize	= atoi(optarg);	break;
            case 'n':	global.ncnt	= atoi(optarg);	break;
	    case 't':	strncpy( cfg.cmd,  optarg, sizeof(cfg.cmd));
		/* fall thru is ok */
	    case 'T':
		cfg.pcmd = cfg.cmd;
		break;
	    case 'u':	cfg.user = optarg;              break;
	    case 'p':	cfg.passwd = optarg;            break;
	    default:
                usage();
                exit(1);
        }
    }

    /*
     *  store the own phone number in global.CalledPartyNumber 
     *  and global.CalledPartySubaddress
     */
    if (cfg.loctelno) {
	cfg.loctelno = strtok(cfg.loctelno, ":/\r\n");
	setCalledPartyNumber(cfg.loctelno);
	if (cfg.loctelno) {
	    cfg.loctelno = strtok(NULL, ":/\r\n");
	    setCalledPartySubaddress(cfg.loctelno);
	}
    }

    /* 
     *  move into the background
     */
    if (cfg.fork_flag) {
	 switch (fork()) {
	     case -1:
		 perror("fork failed");
		 break;
	     case 0:                        /* child */
		 break;
	     default:                       /* parent */
		 exit(0);
	 }
    }

    printf("%s\n", prog_logo);

    /*
     *  create a logfile, if the option -f was used
     */
    if (cfg.logfile) {
	if( !freopen( cfg.logfile, "a+", stdout ) ||
	    !freopen( cfg.logfile, "a+", stderr ) 	) {
	    perror(cfg.logfile);
	    return 1;
	}
    }
    /* output completely  unbuffered */
    setbuf(stdout, NULL);	
    setbuf(stderr, NULL);
    
    /*
     *  create parameter for connect_resp
     */
    setBprotocolFAX(1, 0, cfg.stationid);

    /*
     *  check the controller capabilities
     */
    if (capi2_get_profile( cfg.byCapiContr, &profile_str)) {
	capi2_perror("capi2_get_profile", capi2_errno);
	exit(1);
    }

    if (    !(profile_str.b1protocol & (1<<B1FAXG3) ) ||
	    !(profile_str.b2protocol & (1<<B2T30  ) ) ||
	    !(profile_str.b3protocol & (1<<B3T30  ) )    ) {
	   fprintf( stderr,
		   "sorry, controller %d cannot use the fax G3 protokoll!\n", cfg.byCapiContr);
	   exit(1);
       }

    cfg.fd = init_capi();
#ifdef RECOVER_BRICK_BOOT
    alarm(DEFAULT_KEEP_ALIVE_TIMER);	/* check is brick still alive ??? */
#endif
		      
    /*
     *  poll for capi messages
     */
    pfd.fd      = cfg.fd;
    pfd.events  = POLLIN;
    pfd.revents = 0;
 
    while (endloop <= 0 || cfg.fork_flag ) {
        switch (poll(&pfd, 1, -1)) {
            case -1:
                if (errno == EINTR)  continue;
                if (errno == EAGAIN) continue;
                exit(1);
            case 0:
                exit(0);
            default:
		if (pfd.revents) {
		    endloop = capi_event (cfg.fd);
		    pfd.revents = 0;
		}
                break;
        }
    }
    printf("global.calls      = 0x%x\n", (int)global.calls);
    printf("cfg.call_accepted = %d\n",   cfg.call_accepted);
    exit(0);
}

