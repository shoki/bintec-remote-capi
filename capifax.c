/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: capifax.c,v $
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


#define MAX_FD				64
#define NEWSTATE( ptrCall, newstate) (ptrCall)->state = newstate;
#define DEFAULT_WAIT4DATACONF		0	/* flag */
#define CMD_LINE_ARGS			"?aqnTC:h:i:l:c:s:w:W:v:t:"

#define MAX_CMD_SIZE			256
#define DEFAULT_TRACE_CMD		"bricktrace -hFA %d"
#define DEFAULT_VERBOSE_LEVEL		2

#define REG_CAPI_MSG_SIZE		1400
#define REG_CAPI_LEVEL3CNT		1
#define REG_CAPI_DATA_BLK_SIZE		1024	/* compat with old BRICKS */
#define REG_WINDOW_SIZE			7	/* TODO with 10 fax aborts */

#define CHILD_KILL_DELAY		30	/* seconds      */
#define REGISTER_FAIL_TIMER		30	/* seconds      */

#undef max
#undef min
#define max(a,b)   (((a)>(b))?(a):(b))
#define min(a,b)   (((a)<(b))?(a):(b))

/**********************************
 *
 **********************************/
CONST char * name		= NULL;
CONST char * prog_logo	= "BIANCA/CAPI 2.0 Fax Send Client";

enum state_e {
	Disconnected,
	D_ConnectPending,
	D_Connected,
	B_ConnectPending,
	Connected,
	B_DisconnectPending,
	D_DisconnectPending,
};


/**********************************
 *  call specific data
 **********************************/
typedef struct call_s call_t;
struct call_s {
    call_t		*next;

    /*
     *  name and handle for file to transmit
     */
    CONST char		*pFilename;
    FILE		*txfp;

    /*
     * static configuration data
     */
    struct userdata 	CallingPartyNumber;
    struct userdata 	CalledPartyNumber;
    struct userdata	CallingPartySubaddress;
    struct userdata 	CalledPartySubaddress;
    struct userdata	Bprotocol;

    int 		Cip;

    /*
     * dynamic link data
     */
    int			capi_fd;
    enum state_e	state;
    unsigned short	messid;		/*  message number		*/
    unsigned long 	ident;		/*  contrl, plci, ncci 		*/

    unsigned		active:1;	/*  active, passive side	*/
    unsigned		doSend:1;
    unsigned		wasConnected:1;
    unsigned		NotAcknowledged;/* count datab3_req not ack	*/
};


/**********************************
 *  global data
 **********************************/

struct {
    int 		endloop;
    call_t	 	*calls;
} global = {
    0,
    NULL
};

struct {
    char 		headline[80];
    char		stationid[20];
    char 		rmttelno[40];
    char 		loctelno[40];
    char 		rmtsubaddr[40];
    char 		locsubaddr[40];
    int			verbose;
    int			SffFormat;
    int			speed;
    int 		controller;
    int 		cip;
    int			b_channel;
    char       		*pcmd;	
    char		cmd[MAX_CMD_SIZE];
    int			pid;
    time_t		starttime;
    time_t		endtime;
    unsigned short	usWindowSize;
    int			wait4dataconf;	
} cfg = {
	"BinTec BIANCA/CAPI 2.0 Faxsend",	/*  fax header line	*/
	"+49 911 12345678",			/*  station id		*/
	"",					/*  remote telno	*/
	"",					/*  local telno		*/
	"",					/*  remote subaddr	*/
	"",					/*  local subaddr	*/
	DEFAULT_VERBOSE_LEVEL,			/*  verbose mode 	*/
	0,					/*  SFF format		*/
	0,					/*  maximum speed 	*/
	1,					/*  controller		*/
	17,					/*  FAX G3 CIP		*/
	-1,					/*  b-channel number    */
	NULL,					/*  ptr to cmd string   */
	DEFAULT_TRACE_CMD,			/*  default trace cmd   */
	0,					/*  store pid of cmd    */
	0,					/*  store start time    */
	0,					/*  store end time    	*/
	REG_WINDOW_SIZE,			/*  Window Size		*/
	DEFAULT_WAIT4DATACONF 			/*  flag 		*/
};


struct pollag {
    int (*func)PROTO((int));
};

static int npollfds;
static struct pollfd pollfds[MAX_FD];
static struct pollag pollags[MAX_FD];

/*******************************************************************
 * local proto's
 *******************************************************************/
VOID catch_signal 	PROTO((int signo));
int start_external	PROTO((VOID));
int pollset		PROTO((int fd, int events, int (*func)(int)));
int pollloopt		PROTO(( long t ));
int mypolldel		PROTO(( int fd ));
VOID enqueue_call	PROTO(( call_t *ptrCall ));
int dequeue_call	PROTO(( call_t *ptrCall ));
call_t *alloc_call	PROTO((VOID));
VOID free_call		PROTO(( call_t *ptrCall ));
call_t *getCallbyMessid	PROTO(( int messid ));
call_t *getCallbyPlci	PROTO(( unsigned long plci ));
call_t *getCallbyNcci	PROTO(( unsigned long ncci ));
struct userdata *setCalledPartyNumber  PROTO(( call_t *ptrCall, char *szCalledPartyNumber));
struct userdata *setCallingPartyNumber PROTO(( call_t *ptrCall, char *szCallingPartyNumber, int lPresentation));
struct userdata *setCalledPartySubaddress PROTO(( call_t *ptrCall, char *szCalledPartySubaddress));
struct userdata *setCallingPartySubaddress PROTO(( call_t *ptrCall, char *szCallingPartySubaddress));
struct userdata *setBprotocolFAX  PROTO(( call_t *ptrCall, int  lHighResolution, int  nFormat, char *szStationID, char *szHeaderLine, int  speed));
int getCapiInfo		PROTO(( union CAPI_primitives *capi ));
VOID doDisconnect	PROTO(( call_t *ptrCall ));
VOID SendData		PROTO(( call_t *ptrCall ));
VOID handleConnectB3ActiveInd	PROTO(( call_t *ptrCall ));
int handleDataB3Ind	PROTO(( call_t *ptrCall, unsigned short messid, unsigned char *dataptr, int datalen, unsigned short handle, unsigned short flags ));
VOID handleDataB3Conf	PROTO(( call_t *ptrCall, unsigned short messid, unsigned short handle));
VOID handleInfoInd	PROTO(( call_t *ptrCall, unsigned short infonumber, struct userdata *data));
VOID handleDisconnectB3Ind	PROTO(( call_t *ptrCall, unsigned short nReason, struct userdata *ncpi));
VOID handleDisconnectInd	PROTO(( call_t *ptrCall, unsigned short nReason));
int capi_event		PROTO(( int fd ));
call_t *getCallbyIdent	PROTO(( unsigned long ident ));
int usage		PROTO(( VOID ));
int init_capi		PROTO(( VOID ));
int init_program        PROTO((int argc, char **argv));
int main		PROTO(( int argc, char **argv ));

#ifdef __STDC__
void  my_exit	  ( void );
#else
VOID my_exit	  PROTO(( int *func, caddr_t arg));
#endif

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
VOID catch_signal(signo)
    int signo;
{
    if(signo){
	fprintf( stderr, "\n%s: signo=(%d)\n", name, signo);
    }
    my_kill();
    exit ( -1 );
}
/*******************************************************************
 *
 *******************************************************************/
#ifdef __STDC__
void  my_exit( void  )
#else
VOID my_exit( func, arg)
    int	       *func;
    caddr_t	arg;
#endif
{
    my_kill();
}

/*******************************************************************
 *
 *******************************************************************/
int start_external(VOID)
{
    char	buf[MAX_CMD_SIZE+20];
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
 *
 *******************************************************************/
int pollset( fd, events, func)
int fd; 
int events;
int (*func)PROTO((int));
{
    int i;
    struct pollfd *pfp;
    struct pollag *pap;
 
    if (npollfds >= MAX_FD) return -1;
 
    for (i=0, pfp=pollfds, pap=pollags; i < npollfds; ++i, ++pfp, ++pap) {
        if (pfp->fd == fd && pfp->events == events) break;
    }
    if ( i < npollfds) {
        printf("poll already set !!\n");
        return 0;
    }
 
    pfp = pollfds + npollfds;
    pap = pollags + npollfds;
 
    pfp->fd      = fd;
    pfp->events  = events;
    pfp->revents = 0;
    if (poll(pfp, 1, 0) == -1) return -1;
 
    pap->func    = func;
 
    npollfds ++;
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int pollloopt(t)
long t;
{
    struct pollfd *pfp;
    struct pollag *pap;
    int i;
 
    while (npollfds > 0) {
        switch (poll(pollfds, npollfds, t)) {
            case -1:
                if (errno == EINTR) continue;
                if (errno == EAGAIN) continue;
                return -1;
            case 0:
                return 0;
            default:
                pfp = pollfds;
                pap = pollags;
                for (i=0; i < npollfds; ++i, ++pfp, ++pap) {
                    if (pfp->revents) {
                        (*pap->func)(pfp->fd);
                        pfp->revents = 0;
                    }
                }
                return 0;
                break;
        }
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int mypolldel(fd)
int fd;
{
    struct pollfd *pfp;
    struct pollag *pap;
    int i;
 
    for (i=0, pfp=pollfds, pap=pollags; i < npollfds; ++i, ++pfp, ++pap) {
        if (pfp->fd == fd) break;
    }
 
    if (i >= npollfds) {
        errno = ENOENT;
        return -1;
    }
 
    for (++i, ++pfp, ++pap; i < npollfds; ++i, ++pfp, ++pap) {
        pfp[-1] = pfp[0];
        pap[-1] = pap[0];
    }
    npollfds --;
    return 0;
}
 



/*******************************************************************
 *
 *******************************************************************/
VOID enqueue_call( ptrCall )
call_t *ptrCall;
{
    ptrCall->next = global.calls;
    global.calls = ptrCall;
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
call_t *alloc_call(VOID)
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
	if (ptrCall->txfp) {
	    fclose( ptrCall->txfp);
	}
	free(ptrCall);
    }
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyMessid( messid )
int messid;
{
    call_t *ptrCall;

    for(ptrCall = global.calls; ptrCall; ptrCall = ptrCall->next) {
	if (ptrCall->messid == messid) return ptrCall;
    }
    if(cfg.verbose >8) {
	printf("getCallbyMessid(0x%x) return == NULL\n", messid);
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyPlci( plci )
unsigned long plci;
{
    call_t *ptrCall;
    unsigned long plci1 = plci;

    plci &= 0xffff;
    for(ptrCall = global.calls; ptrCall; ptrCall = ptrCall->next) {
	if ((ptrCall->ident & 0xffff) == plci) return ptrCall;
    }
    if(cfg.verbose >8) {
	printf("getCallbyPlci(0x%lx)->(0x%lx) return == NULL\n", plci1, plci);
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
    if(cfg.verbose >8) {
	printf("getCallbyNcci(0x%lx) return == NULL\n", ncci);
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyIdent( ident )
unsigned long ident;
{
    call_t *ptrCall = NULL;

    if(cfg.verbose >9) {
	printf("getCallbyIdent(0x%0lx)\n", ident);
    }
    if (ident >= 0xffff) {
	ptrCall = getCallbyNcci( ident );
    }
    else if (ident >= 0xff) {
	ptrCall = getCallbyPlci( ident );
    }
    else {
	if(cfg.verbose >8) {
	    printf("getCallbyIdent(0x%0lx) return == NULL\n", ident);
	}
    }

    return (ptrCall);
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCalledPartyNumber( ptrCall, szCalledPartyNumber )
call_t *ptrCall;
char *szCalledPartyNumber;
{
    size_t len = 0;

    ptrCall->CalledPartyNumber.length = 0;
    if (szCalledPartyNumber) {
	len = strlen( szCalledPartyNumber);
	len = min(len, CAPI1_MAXMSGLEN-1);
	if (len) {
	    ptrCall->CalledPartyNumber.data[0] = 0x81;

	    memcpy( &ptrCall->CalledPartyNumber.data[1],
		    szCalledPartyNumber, len);
	    ptrCall->CalledPartyNumber.length = len+1;
	}
    }

    return &ptrCall->CalledPartyNumber;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCallingPartyNumber( ptrCall, szCallingPartyNumber, lPresentation )
call_t *ptrCall;
char *szCallingPartyNumber;
int lPresentation;
{
    size_t len = 0;

    ptrCall->CallingPartyNumber.length = 0;
    if (szCallingPartyNumber) {
	len = strlen( szCallingPartyNumber);
	len = min(len, CAPI1_MAXMSGLEN-1);
	if (len) {
	    ptrCall->CallingPartyNumber.data[0] = 0x01;
	    ptrCall->CallingPartyNumber.data[1] = lPresentation ? 0x80 : 0xa0;

	    memcpy( &ptrCall->CallingPartyNumber.data[2], 
		    szCallingPartyNumber, len);
	    ptrCall->CallingPartyNumber.length = len+2;
	}
    } 

    return &ptrCall->CallingPartyNumber;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCalledPartySubaddress( ptrCall, szCalledPartySubaddress)
call_t *ptrCall;
char *szCalledPartySubaddress;
{
    size_t len;

    if (szCalledPartySubaddress) {
	len = strlen(szCalledPartySubaddress);
	ptrCall->CalledPartySubaddress.length  = len + 1;
	ptrCall->CalledPartySubaddress.data[0] = 0x80;

	memcpy( &ptrCall->CalledPartySubaddress.data[1],
		szCalledPartySubaddress, len);

    } else {
	ptrCall->CalledPartySubaddress.length = 0;
    }
    return &ptrCall->CalledPartySubaddress;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setCallingPartySubaddress( ptrCall, szCallingPartySubaddress)
call_t *ptrCall;
char *szCallingPartySubaddress;
{
    size_t len;

    if (szCallingPartySubaddress) {
	len = strlen(szCallingPartySubaddress);
	ptrCall->CallingPartySubaddress.length  = len + 1;
	ptrCall->CallingPartySubaddress.data[0] = 0x80;

	memcpy( &ptrCall->CallingPartySubaddress.data[1],
		szCallingPartySubaddress, len);
    } else {
	ptrCall->CallingPartySubaddress.length = 0;
    }
    return &ptrCall->CallingPartySubaddress;
}

/*******************************************************************
 *
 *******************************************************************/
struct userdata *setBprotocolFAX( ptrCall, lHighResolution, nFormat,
				    szStationID, szHeaderLine, speed)
call_t *ptrCall;
int  lHighResolution;
int  nFormat;
char *szStationID;
char *szHeaderLine;
int  speed;
{
    struct bprotocol *bprot;
    struct userdata *data;
    struct b3config_faxg3 *b3cfg;
    struct b1config *b1cfg;

    bprot = (struct bprotocol *)&ptrCall->Bprotocol;

    PUT_WORD( bprot->b1proto, B1FAXG3);
    PUT_WORD( bprot->b2proto, B2T30);
    PUT_WORD( bprot->b3proto, B3T30);

    data = (struct userdata *)&bprot->structlen;
    b1cfg = (struct b1config *) data;
    PUT_WORD( b1cfg->rate, speed);
    PUT_WORD( b1cfg->bpc, 0);
    PUT_WORD( b1cfg->parity, 0);
    PUT_WORD( b1cfg->stopbits, 0);
    b1cfg->length = sizeof( struct b1config) - 1;

    data = (struct userdata *)&data->data[data->length];
    data->length = 0;	/*  b2config  */
    data = (struct userdata *)&data->data[data->length];
    b3cfg = (struct b3config_faxg3 *)data;

    PUT_WORD( b3cfg->resolution, lHighResolution ? 1 : 0);
    PUT_WORD( b3cfg->format,     nFormat);	

    data = (struct userdata *)&b3cfg->structlen;
    data->length = strlen(szStationID);
    memcpy( data->data, szStationID, data->length);

    data = (struct userdata *)&data->data[data->length];
    data->length = strlen(szHeaderLine);
    memcpy( data->data, szHeaderLine, data->length);

    data = (struct userdata *)&data->data[data->length];

    b3cfg->length = (char *)data - (char *)&b3cfg->length - 1;
    bprot->length = (char *)data - (char *)&bprot->length - 1;

    return (struct userdata *)bprot;
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
	case CAPI2_DISCONNECT_IND:
	    info = GET_WORD(capi->c2disconnect_ind.reason);
	    break;
	case CAPI2_DISCONNECTB3_IND:
	    info = GET_WORD(capi->c2disconnectb3_ind.reason_b3);
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
VOID doDisconnect( ptrCall)
call_t *ptrCall;
{
    unsigned long plci, ncci;

    ncci = ptrCall->ident;
    plci = ptrCall->ident & 0xffff;

    switch (ptrCall->state) {
	case B_ConnectPending:
	case Connected:
	    if(cfg.wait4dataconf && ptrCall->NotAcknowledged) {
		if(cfg.verbose>1) {
		    printf("doDisconnect() wait for %d CAPI2_DATAB3_CONF\n", 
			    ptrCall->NotAcknowledged);
		}
		/* don't send a capi2_disconnectb3_req() we have to wait */
		break;
	    }
	    if(ptrCall->NotAcknowledged && cfg.verbose>0) {
		printf("doDisconnect() missing %d CAPI2_DATAB3_CONF\n", 
			ptrCall->NotAcknowledged);
	    }
	    capi2_disconnectb3_req( ptrCall->capi_fd,
				    ncci,
				    NULL);
	    break;

	case D_ConnectPending:
	case B_DisconnectPending:
	    NEWSTATE( ptrCall, D_DisconnectPending);
	    capi2_disconnect_req( ptrCall->capi_fd,
				  plci,
				  NULL);

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
VOID SendData( ptrCall)
call_t *ptrCall;
{
    static unsigned short usDataHandle;
    char		buffer[REG_CAPI_DATA_BLK_SIZE];
    int			len;

    if (0== ptrCall->doSend) { return; }

    /*
     * Read next blk from file into buffer
     */
    len = fread( buffer, 1, sizeof(buffer), ptrCall->txfp);
    if (len <= 0) {
	if (len < 0) {
	    perror("\nERR:SendData\n");
	}
	ptrCall->doSend = 0;
	printf("|\n");
	if (cfg.verbose > 2) {
	    printf("Send %d packets with size %d\n",
		    usDataHandle, REG_CAPI_DATA_BLK_SIZE);
	}
	doDisconnect( ptrCall );
	return;
    }
    /*
     * Now send next data blk
     */
    if(cfg.verbose > 1) {
	putchar('*');
	fflush(stdout);
    }

    ptrCall->NotAcknowledged++;		/* count pending confirms */
    usDataHandle++;
    capi2_datab3_req( ptrCall->capi_fd,
	    ptrCall->ident,
	    buffer,
	    len,
	    0,
	    usDataHandle);

}

/*******************************************************************
 *
 *******************************************************************/
VOID handleConnectB3ActiveInd( ptrCall )
call_t *ptrCall;
{
    unsigned cnt;

    if(cfg.verbose > 0) {
	printf("Connected to ISDN no <%s>\n", cfg.rmttelno);
	printf("Send FAX Data: ");
	fflush(stdout);
    }
    if (!ptrCall->doSend) {
	cfg.starttime		= time(0);
	ptrCall->doSend		= 1;
	ptrCall->wasConnected	= 1;
	if(cfg.verbose > 1) {
	    putchar('|');
	    fflush(stdout);
	}
	SendData( ptrCall );
    } else {
	printf("Already sending!\n");
    }
}


/*******************************************************************
 *
 *******************************************************************/
int handleDataB3Ind( ptrCall, messid, dataptr, datalen, handle, flags)
    call_t		*ptrCall;
    unsigned short	messid;
    unsigned char	*dataptr;
    int			datalen;
    unsigned short	handle;
    unsigned short	flags;
{
    printf("DataB3Ind ??? not supported by %s\n", name);
    return 1;	/*  response */
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleDataB3Conf( ptrCall, messid, handle )
    call_t		*ptrCall;
    unsigned short	messid;
    unsigned short	handle;
{
    ptrCall->NotAcknowledged--;		/* We got a acknowledge */
    if(cfg.verbose > 2) {
	putchar('c');
	fflush(stdout);
    }
    SendData( ptrCall );		/* Send the next one	*/
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleInfoInd( ptrCall, infonumber, data)
    call_t		*ptrCall;
    unsigned short	infonumber;
    struct userdata	*data;
{

    unsigned long	mask;
    unsigned char      *datap = data->data;

    switch(infonumber){
	default:
	    if(cfg.verbose>5){
	        printf("\tINFOIND: 0x%04x\n", infonumber);
		capi_hexdump( data->data, data->length, 16, 2);
	    }
	    break;
	case 0x18:	/* IBCHI	*/
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
		cfg.pcmd  = NULL;	/* do it only once */
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
    struct userdata	*ncpi;
{
    struct ncpi2_fax	*fax = (struct ncpi2_fax *)ncpi;
    int			delta_time;

    cfg.endtime = time(0);
    delta_time  = cfg.endtime - cfg.starttime;

    printf("\nFAX T.30 Disconnect\n");
    if (nReason) printf("\tB3_Reason (0x%04x)\n", nReason);

    if (fax->length >= sizeof(*fax)) {
	printf("\tRemote Station ID : '%*.*s'\n", fax->structlen,
		fax->structlen,
		&fax->structlen+1);

	printf("\tTransfer-Rate     : %d bps\n", GET_WORD(fax->rate));
	printf("\tResolution        : %s\n",     GET_WORD(fax->resolution) 
		? "high (196 DPI)" 
		: "low (98 DPI)");
	printf("\tNumber of pages   : %d\n",     GET_WORD(fax->pages));
	printf("\tTransfer-Time     : %d:%d (min:sec)\n", 
		delta_time / 60, delta_time % 60);
	printf("\n");
    }
    else {
	printf("\tWRONG LEN ???\n");
	goto handleDisconnectB3Ind_dump;
    }
    if(cfg.verbose > 5) {
handleDisconnectB3Ind_dump:
	printf("\tstruct ncpi2_fax length=%ld\n", (size_t)fax->length);
	capi_hexdump((char*)fax, (size_t)fax->length, 0x10, 2);
	fflush(stdout);
    }
    if(ptrCall->NotAcknowledged && cfg.verbose>0) {
	printf("handleDisconnectB3Ind() missing %d CAPI2_DATAB3_CONF\n", 
		ptrCall->NotAcknowledged);
    }
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleDisconnectInd( ptrCall, nReason)
    call_t *ptrCall;
    unsigned short nReason;
{
    printf("ISDN D-channel disconnect, Reason (%04x)\n", nReason);
    printf("\n");
}


/*******************************************************************
 *
 *******************************************************************/
int capi_event(fd)
    int fd;
{
    int			info;
    int			lcause;
    unsigned long	ident;
    unsigned char	proto[1024];
    unsigned char	data[4096];
    union CAPI_primitives *capi;
    call_t *ptrCall;

    lcause = capi2_get_message( fd, &capi,
	    (union CAPI_primitives *)proto, data, sizeof(data));

    /* 
     *  check for errors
     */
    switch (lcause) {
	case CAPI2_E_MSG_QUEUE_EMPTY:	 	return  0;		
	case CAPI2_E_INTERNAL_BUSY_CONDITION:	return -1;
	default:				break;
    }

    info    = getCapiInfo(capi);
    if (info) capi2_perror("\nCAPI Info", info);

    /*
     *  get the call structure
     */
    ident   = GET_c2IDENT(capi);
    ptrCall = getCallbyIdent( ident);

    switch (GET_PRIMTYPE(capi)) {
	case CAPI2_LISTEN_CONF:
	    break;
	
	case CAPI2_CONNECT_CONF:
	    ptrCall = getCallbyMessid(GET_MESSID(capi));
	    if (ptrCall) {
		ptrCall->ident = ident;
		if (info) {	/* CONNECT_REQ failed -> end program */ 
		    mypolldel( ptrCall->capi_fd );
		    free_call( ptrCall );
		    global.endloop = 1;
		}
	    }	
	    break;
	
	case CAPI2_INFO_IND:
	    capi2_info_resp( fd, GET_MESSID(capi), ident);
	    if (ptrCall) {
		handleInfoInd( ptrCall,
			GET_WORD(capi->c2info_ind.info),
			(struct userdata *)&capi->c2info_ind.structlen);
	    }
	    break;
	
	case CAPI2_CONNECTACTIVE_IND:
	    if (cfg.verbose>8) {
		printf("CAPI2_CONNECTACTIVE_IND %lx\n", ident);
	    }
	    capi2_connectactive_resp( fd, GET_MESSID(capi), ident);
	    if (ptrCall) {
		NEWSTATE( ptrCall, D_Connected);

		if (ptrCall->active) {
		    NEWSTATE( ptrCall, B_ConnectPending);
		    capi2_connectb3_req( fd, ident, NULL);
		}
	    }
	    break;
	    
	case CAPI2_CONNECTB3_CONF:
	    ptrCall = getCallbyPlci( ident & 0xffff );
	    if (ptrCall) {
		/*
		 * NCCI allocation -> store NCCI for later use
		 */
		ptrCall->ident = ident;
	    }
	    break;
	    
	case CAPI2_CONNECTB3ACTIVE_IND:
	    capi2_connectb3active_resp( fd, GET_MESSID(capi), ident);
	    if (ptrCall) {
		NEWSTATE( ptrCall, Connected);
		handleConnectB3ActiveInd( ptrCall );
	    }
	    break;

	case CAPI2_DATAB3_CONF:
	    if (ptrCall) {
		handleDataB3Conf( ptrCall, 
				  GET_MESSID(capi),
				  GET_WORD(capi->c2datab3_conf.handle));
	    }
	    break;

	case CAPI2_DATAB3_IND:
	    if (ptrCall) {
		if (handleDataB3Ind( ptrCall,
				     GET_MESSID(capi),
				 (char *)GET_DWORD(capi->c2datab3_ind.dataptr),
				     GET_WORD( capi->c2datab3_ind.datalen),
				     GET_WORD( capi->c2datab3_ind.handle),
				     GET_WORD( capi->c2datab3_ind.flags))) {
		    capi2_datab3_resp( fd,
				       GET_MESSID(capi),
				       ident,
				       GET_WORD(capi->c2datab3_ind.handle));
		}
	    }
	    break;

	case CAPI2_DISCONNECTB3_IND:
	    if (ptrCall) {
		NEWSTATE( ptrCall, B_DisconnectPending);
		handleDisconnectB3Ind( ptrCall, 
				GET_WORD(capi->c2disconnectb3_ind.reason_b3),
		    (struct userdata *)&capi->c2disconnectb3_ind.structlen);
	    }	
	    capi2_disconnectb3_resp( fd,
				     GET_MESSID(capi),
				     ident);
	    if (ptrCall) NEWSTATE( ptrCall, D_DisconnectPending);
	    capi2_disconnect_req( fd,
				  ident & 0xffff,
				  NULL);
	    break;
		
	case CAPI2_DISCONNECT_IND:
	    capi2_disconnect_resp( fd,
				   GET_MESSID(capi),
				   ident);

	    if (ptrCall) {
		NEWSTATE( ptrCall, Disconnected);
		handleDisconnectInd( ptrCall, 
				     GET_WORD(capi->c2disconnect_ind.reason));

		mypolldel( ptrCall->capi_fd );
		free_call( ptrCall );
	    }
	    global.endloop = 1;
	    break;

	case CAPI2_ALERT_CONF:
	case CAPI2_INFO_CONF:
	case CAPI2_DISCONNECTB3_CONF:
	case CAPI2_DISCONNECT_CONF:
	case CAPI2_FACILITY_CONF:
	    break;
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int usage( VOID )
{
    fprintf( stderr, "Usage: %s [-%s] <faxno> [file]\n", name, CMD_LINE_ARGS);
    fprintf( stderr, "\tOptions:\n");
    fprintf( stderr, "\t-h: specify user part of fax header line\n");
    fprintf( stderr, "\t-i: specify local station identification\n");
    fprintf( stderr, "\t-s: specify maximum transfer speed in bps\n");
    fprintf( stderr, "\t-c: specify ISDN controller to use [1..n]\n");
    fprintf( stderr, "\t-a: use ascii mode\n");
    fprintf( stderr, "\t-q: use quiet mode\n");
    fprintf( stderr, "\t-v: specify verbose level (default=%d)\n",
	    DEFAULT_VERBOSE_LEVEL);
    fprintf( stderr, "\t-n: suppress BIANCA/FAX headline\n");
    fprintf( stderr, "\t-w: specify data window size (default=%d)\n",
	    REG_WINDOW_SIZE);
    fprintf( stderr, "\t-W: wait for DATAB3_CONF before DISCONNECTB3_REQ (default=%d)\n",
	    DEFAULT_WAIT4DATACONF);
    fprintf( stderr, "\t-t: exec <cmd %%d> external program when bchannel established\n");
    fprintf( stderr, "\t-T: <exec %s> when bchannel established\n" ,
	    DEFAULT_TRACE_CMD );
    fprintf( stderr, "\texample:\n\t%s -t'bricktrace -Hbrick1 -hFA %%d 0 3' ...\n", name);
    fprintf( stderr, "\t%s -t'bricktrace -Hbrick1 -hFA %%d 0 3 >trace%%d.log' ...\n", name);
    fprintf( stderr, "\n\tif file is not specified, stdin is used\n");

    return 1;
}

/*******************************************************************
 *
 *******************************************************************/
int init_capi(VOID)
{
    int		fd;
    
    for(;;) { 
	if(cfg.verbose > 2){
	    putchar('R');
	    fflush(stdout);
	}

    fd = capi2_register( REG_CAPI_MSG_SIZE,
				REG_CAPI_LEVEL3CNT,
				cfg.usWindowSize,
				REG_CAPI_DATA_BLK_SIZE,
				NULL);

	if(fd >0 ) break;
	fprintf( stderr, "capi2_register failed! retry in %d seconds\n",
		REGISTER_FAIL_TIMER     );
	sleep(REGISTER_FAIL_TIMER);
    }
    return fd;
}   


/*******************************************************************
 *
 *******************************************************************/
int init_program(argc, argv)
    int argc;
    char **argv;
{
    name = argv[0];
    printf("%s\n", prog_logo);

#ifdef __STDC__
    atexit( my_exit );
#else
    on_exit( my_exit, 0);
#endif
    signal(SIGHUP,	catch_signal);
    signal(SIGINT,	catch_signal);
    signal(SIGQUIT,	catch_signal);
    signal(SIGKILL,	catch_signal);
    signal(SIGTERM,	catch_signal);
    signal(SIGUSR1,	catch_signal);
    signal(SIGUSR2,	catch_signal);

    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
int main(argc, argv)
int argc;
char **argv;
{
    call_t	*ptrCall;
    int		i;
    int		max_controller;
    unsigned long controller_mask = 0;
    struct capi_getprofile profile;

    extern int		optind;
    extern char 	*optarg;

    init_program(argc, argv);

    /*
     * get options
     */
    while ((i = getopt( argc, argv, CMD_LINE_ARGS )) != EOF) switch (i) {
	case 'h': strncpy( cfg.headline,  optarg, sizeof(cfg.headline));  break;
	case 'i': strncpy( cfg.stationid, optarg, sizeof(cfg.stationid)); break;
	case 'l': strncpy( cfg.loctelno,  optarg, sizeof(cfg.loctelno));  break;
	case 'c': cfg.controller    = atoi(optarg);			  break;
	case 's': cfg.speed         = atoi(optarg);		    	  break;
	case 'a': cfg.SffFormat    |= 5;			  	  break;
	case 'n': cfg.SffFormat    |= 128;			  	  break;
	case 'q': cfg.verbose       = 0;			  	  break;
	case 'v': cfg.verbose       = atoi(optarg);		    	  break;
	case 'C': cfg.cip           = atoi(optarg);			  break;
	case 'w': cfg.usWindowSize  = atoi(optarg);			  break;
	case 'W': cfg.wait4dataconf = atoi(optarg);			  break;
	case 't': strncpy( cfg.cmd,  optarg, sizeof(cfg.cmd));
		  /* fall thru is ok */
	case 'T': cfg.pcmd = cfg.cmd;			 		  break;
	default:  usage(); exit(1); 
    }

    if (optind < argc) {
	strncpy( cfg.rmttelno, argv[optind++], sizeof(cfg.rmttelno));
    } else {
	usage();
	exit(1);
    }

    ptrCall = alloc_call();
    if (!ptrCall) {
	fprintf( stderr, "alloc_call failed!\n");
	return 1;
    }
    enqueue_call( ptrCall );

    /*
     * open file to transmit or
     * use stdin
     */
    ptrCall->pFilename  = "<stdin>";
    ptrCall->txfp       = stdin;
    if (optind < argc) {
	ptrCall->pFilename = argv[optind];
	if (!(ptrCall->txfp=fopen( ptrCall->pFilename, "r"))) {
	    return 1;
	}
    }
    if ( cfg.verbose > 2 ) {
	printf("Reading from file: <%s>\n", ptrCall->pFilename);
    }

    memset( &profile, 0, sizeof(profile));
    capi2_get_profile( 0, &profile);
    max_controller = profile.ncontrl;

    printf("Available BIANCA/CAPI ISDN controllers: %d\n", max_controller);

    if (max_controller) {
	for( i=1; i<33; i++) {
	    memset( &profile, 0, sizeof(profile));
	    if (!capi2_get_profile( i, &profile)) {
		controller_mask |= (1 << (i-1));
		if (!--max_controller) break;
	    }
	}
    }

    if (!(((1 << cfg.controller)-1) & controller_mask)) {
	fprintf( stderr, "No such Controller %d!\n", cfg.controller);
	return 1;
    }
    memset( &profile, 0, sizeof(profile));
    capi2_get_profile( cfg.controller, &profile);

#if 0	/* debug printf */
    fprintf( stderr, "ncontrl    = 0x%x\n", profile.ncontrl); 
    fprintf( stderr, "nchannel   = 0x%x\n", profile.nchannel); 
    fprintf( stderr, "options    = 0x%x\n", profile.options); 
    fprintf( stderr, "b1protocol = 0x%x\n", profile.b1protocol);
    fprintf( stderr, "b2protocol = 0x%x\n", profile.b2protocol);
    fprintf( stderr, "b3protocol = 0x%x\n", profile.b3protocol);
    fprintf( stderr, "\n");
#endif
    if ( !(profile.b1protocol & (1 << 4)) ||
	 !(profile.b2protocol & (1 << 4)) ||
	 !(profile.b3protocol & (1 << 4))) {
	fprintf( stderr, "Controller %d: does not support FAX G3\n",
						cfg.controller);
	return 1;
    }

    /*
     * do register 
     */
	
    ptrCall->capi_fd = init_capi();
    if (ptrCall->capi_fd <= 0) {
	fprintf( stderr, "capi2_register failed!\n");
	return -1;
    }

    ptrCall->active = 1;
    ptrCall->ident  = cfg.controller;	/*  controller  */
    ptrCall->Cip    = cfg.cip;

    setCalledPartyNumber(      ptrCall, cfg.rmttelno);
    setCallingPartyNumber(     ptrCall, cfg.loctelno, 1);
    setCalledPartySubaddress(  ptrCall, cfg.rmtsubaddr);
    setCallingPartySubaddress( ptrCall, cfg.locsubaddr);
    setBprotocolFAX(           ptrCall, 1, cfg.SffFormat, 
					   cfg.stationid,
					   cfg.headline,
					   cfg.speed); 
    capi2_listen_req( ptrCall->capi_fd,		
		      ptrCall->ident & 0x7f,	/* Controller */
		      0x0000017f,		/* Info Mask */
		      0,			/* CIP Mask */
		      0,			/* CIP Mask2 */
		      NULL,			/* Calling party number */
		      NULL);			/* Calling party subaddress */


    printf("Trying...\n");
    ptrCall->messid = capi2_connect_req( ptrCall->capi_fd,
		                         ptrCall->ident & 0x7f,
		                         ptrCall->Cip,
		                         &ptrCall->CalledPartyNumber,
		                         &ptrCall->CallingPartyNumber,
		                         &ptrCall->CalledPartySubaddress,
		                         &ptrCall->CallingPartySubaddress,
		                         &ptrCall->Bprotocol,
		                         NULL, NULL, NULL,
		                         NULL);
    if(cfg.verbose>8) {
	printf("messid from capi2_connect_req() == 0x%x\n", ptrCall->messid);
    }
    NEWSTATE( ptrCall, D_ConnectPending);

    pollset( ptrCall->capi_fd, POLLIN, capi_event);

    while (!global.endloop) {
	if (pollloopt(-1) < 0) break;
    }

    return 0;
}

