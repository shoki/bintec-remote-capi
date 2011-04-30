/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: capitest.c,v $
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
#define USE_POLL 			1	/* 1 -> poll, 0 -> kq */

#define	_GNU_SOURCE			/* use GNU specific options needed for
					 * strsignal (can be removed if not
					 * there) */
#include "capiconf.h"
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>

#if USE_POLL
#else
#include <sys/event.h>
#endif

#include <string.h>			/* for str* functions */


#define MAX_FD				256
#define NEWSTATE( ptrCall, newstate) (ptrCall)->state = newstate;
#define DEFAULT_WAIT4DATACONF		0	/* flag */
#define CMD_LINE_ARGS			"?l:y:p:rs:h:i:a:c:C:B:qv:nw:W:t:T:M:S"

#define MAX_CMD_SIZE			256
#define DEFAULT_TRACE_CMD		"bricktrace -hFA %d"
#define DEFAULT_VERBOSE_LEVEL		2

#define REG_CAPI_MSG_SIZE		1024
#define REG_CAPI_LEVEL3CNT		1
#define REG_CAPI_DATA_BLK_SIZE		512	/* compat with old BRICKS */
#define REG_WINDOW_SIZE			7	/* TODO with 10 fax aborts */

#define	MAX_CHANNELS_PER_TCP_STREAM	10

#define MAX_SEND_BUFFER_SIZE		4096

#define CHILD_KILL_DELAY		30	/* seconds      */
#define REGISTER_FAIL_TIMER		30	/* seconds      */

#define BIT(b)	(1 << (b))

#define LISTEN_CIP_MASK ( 1<<4 )	/* TODO: use BIT macro */

#undef max
#undef min
#define max(a,b)   (((a)>(b))?(a):(b))
#define min(a,b)   (((a)<(b))?(a):(b))

/**********************************
 *
 **********************************/
CONST char * name		= NULL;
CONST char * prog_logo	= "BIANCA/CAPI 2.0 Test Client";

CONST char   send_buffer[MAX_SEND_BUFFER_SIZE + 256];

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
typedef struct _TCapiAppl_s	TCapiAppl_t;
typedef struct call_s call_t;
struct call_s {
    call_t		*pNextCall;
    call_t		*pNextCallHash;
    TCapiAppl_t		*pAppl;

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
    enum state_e	state;
    unsigned short	messid;		/*  message number		*/
    unsigned long 	ident;		/*  contrl, plci, ncci 		*/

    unsigned		active:1;	/*  active, passive side	*/
    unsigned		doSend:1;
    unsigned		wasConnected:1;
    unsigned		NotAcknowledged;/* count datab3_req not ack	*/
    size_t		sentPackets;
    size_t		maxSentPackets;
    size_t		sentBytes;
    size_t		recvdPackets;
    size_t		recvdBytes;
    time_t		starttime;
};

#define NCCI_MAX_HASH		5	/* number of used bits */
#define NCCI2IDX(ncci)	(((ncci)>>16) % (1<<NCCI_MAX_HASH) )

struct _TCapiAppl_s {
    TCapiAppl_t		*pNextAppl;
    int			fd;
    unsigned		listen_mask;
    call_t		*pCalls;
    int			active_calls;
    int			conn_calls;
    call_t		*apCallHash[1<<NCCI_MAX_HASH];
};


/**********************************
 *  global data
 **********************************/

struct {
    int 		endloop;
    int			appl_cnt;
    TCapiAppl_t		*pApplAnchor;	
} global;

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
    int			recv;
    int			MaxActivCalls;
    int			callperappl;
    int			proto;
    int			datab3Size;
    size_t		maxSentPackets;
    int			csvoutput;
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
	LISTEN_CIP_MASK,					/*  FAX G3 CIP		*/
	-1,					/*  b-channel number    */
	NULL,					/*  ptr to cmd string   */
	DEFAULT_TRACE_CMD,			/*  default trace cmd   */
	0,					/*  store pid of cmd    */
	0,					/*  store start time    */
	0,					/*  store end time    	*/
	REG_WINDOW_SIZE,			/*  Window Size		*/
	DEFAULT_WAIT4DATACONF,			/*  flag 		*/
	0,
	0,
	MAX_CHANNELS_PER_TCP_STREAM,
	0,
	REG_CAPI_DATA_BLK_SIZE,
	0,
	0
};

struct pollag {
    int (*func)PROTO((void* arg));
    void	*arg;
};

static int npollfds;
static struct pollfd pollfds[MAX_FD];
static struct pollag pollags[MAX_FD];


/*******************************************************************
 * local proto's
 *******************************************************************/
VOID catch_signal 	PROTO((int signo));
int capi2_options_print PROTO(( unsigned long options ));
int capi2_b1protocol_print PROTO(( unsigned long b1protocol ));
int capi2_b2protocol_print PROTO(( unsigned long b2protocol ));
int capi2_b3protocol_print PROTO(( unsigned long b3protocol ));
int start_external	PROTO((VOID));
int pollset		PROTO((int fd, int events, int (*func)(void *), void*arg));
int pollloopt		PROTO(( long t ));
int mypolldel		PROTO(( int fd ));
int dequeue_call_struct	PROTO(( call_t *ptrCall ));
call_t *lloc_call	PROTO((VOID));
VOID free_call_struct		PROTO(( call_t *ptrCall ));
call_t *getCallbyMessid	PROTO(( TCapiAppl_t *pA, int messid ));
call_t *getCallbyPlci	PROTO(( TCapiAppl_t *pA, unsigned long plci ));
call_t *getCallbyNcci	PROTO(( TCapiAppl_t *pA, unsigned long ncci ));
call_t *getCallbyIdent	PROTO(( TCapiAppl_t *pA, unsigned long ident ));
struct userdata *setCalledPartyNumber  PROTO(( call_t *ptrCall, char *szCalledPartyNumber));
struct userdata *setCallingPartyNumber PROTO(( call_t *ptrCall, char *szCallingPartyNumber, int lPresentation));
struct userdata *setCalledPartySubaddress PROTO(( call_t *ptrCall, char *szCalledPartySubaddress));
struct userdata *setCallingPartySubaddress PROTO(( call_t *ptrCall, char *szCallingPartySubaddress));
struct userdata *setBprotocol PROTO(( call_t *ptrCall, int  lHighResolution, int  nFormat, char *szStationID, char *szHeaderLine, int  speed, int proto ));
int getCapiInfo		PROTO(( union CAPI_primitives *capi ));
VOID doDisconnect	PROTO(( call_t *ptrCall ));
VOID SendData		PROTO(( call_t *ptrCall ));
VOID handleConnectB3ActiveInd	PROTO(( call_t *ptrCall ));
int handleDataB3Ind	PROTO(( call_t *ptrCall, unsigned short messid, unsigned char *dataptr, int datalen, unsigned short handle, unsigned short flags ));
VOID handleDataB3Conf	PROTO(( call_t *ptrCall, unsigned short messid, unsigned short handle));
VOID handleInfoInd	PROTO(( call_t *ptrCall, unsigned short infonumber, struct userdata *data));
VOID handleDisconnectB3Ind	PROTO(( call_t *ptrCall, unsigned short nReason, struct userdata *ncpi));
VOID handleDisconnectInd	PROTO(( call_t *ptrCall, unsigned short nReason));
int capi_event(void *);
void print_call_stats	PROTO((void));
void DisconnectAll	PROTO((void));
void set_listen 	PROTO((TCapiAppl_t *pA, int onoff ));
int usage		PROTO(( VOID ));
TCapiAppl_t *alloc_appl(VOID);
VOID free_appl(TCapiAppl_t *pA);
void check_appl(TCapiAppl_t *pA );
int gen_call(TCapiAppl_t *pA);
void gen_next_call(call_t *pC);
void deattach_call(call_t *pC);
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
void check_appl(TCapiAppl_t *pA )
{
    TCapiAppl_t	*pA_call = NULL;
    pA = pA ? pA: global.pApplAnchor;

    if(!cfg.recv){
	return;
    }

    for(; pA; pA = pA->pNextAppl) {
	if(pA->active_calls < cfg.callperappl) {
	    if(NULL==pA_call) {
		pA_call = pA;
		set_listen(pA, 1);	/* switch on */
		break;
	    }
	}
	else {
	    set_listen(pA, 0);	/* switch off */
	}
    }
    if (NULL==pA_call) {
	/* Create a new one because the others have enough */
	pA_call = alloc_appl();
    }
}
/*******************************************************************
 *
 *******************************************************************/
void gen_next_call(call_t *pC)
{
    TCapiAppl_t	*pA;
    TCapiAppl_t	*pA_call = NULL;
    int		cnt_calls	= 0;

    if(cfg.recv){
	return;
    }

    pA = pC ? pC->pAppl: global.pApplAnchor;
    for(; pA; pA = pA->pNextAppl) {
	cnt_calls += pA->active_calls;

	if(cfg.MaxActivCalls &&(cnt_calls>= cfg.MaxActivCalls)) {
	    printf("All calls are connected now.\n");
	    return;	/* this is enough */
	}
	if(pA->active_calls < cfg.callperappl) {
	    if(NULL==pA_call) {
		pA_call = pA;
	    }
	}
    }
    if (NULL==pA_call) {
	/* Create a new one because the others have enough */
	pA_call = alloc_appl();
    }

    /* only for sending side */
    if (cfg.MaxActivCalls && pA_call) {
	printf("Setup call %d of %d: ",
		cnt_calls + 1, cfg.MaxActivCalls);
	gen_call(pA_call);
    }
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
VOID catch_signal(signo)
    int signo;
{
    switch(signo) {
	case SIGUSR1:
	    print_call_stats();
	    break;
	case SIGUSR2:
	    gen_next_call(NULL);
	    break;
	case SIGHUP:
	    /* if we got calls try to disconnect them */
	    DisconnectAll();
	    break;
	default:
	    if(signo){
		fprintf( stderr, "\n%s: %s\n", name, 
			strsignal(signo));
	    }
	    my_kill();
	    exit ( -1 );
	    break;
    }
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
 * output functions
 *******************************************************************/
int capi2_options_print( unsigned long options )
{
    if (!options) return 0;

    if (options & CAPI2_P_INT     ) printf("INTERNAL ");
    if (options & CAPI2_P_EXT     ) printf("EXTERNAL ");
    if (options & CAPI2_P_HANDSET ) printf("HANDSET ");
    if (options & CAPI2_P_DTMF    ) printf("DTMF ");
    if (options & CAPI2_P_SUPSERV ) printf("SUPPLEMENTARY_SERVICES ");
    if (options & CAPI2_P_LEASED  ) printf("LEASED_LINE ");
    if (options & CAPI2_P_BCHAN   ) printf("B_CHANNEL ");
    if (options & CAPI2_P_LINTCON ) printf("LINE_INTERCONNECT ");
    return 1;
}

int capi2_b1protocol_print ( unsigned long b1protocol ) 
{
    if (!b1protocol) return 0;
    if ( b1protocol & BIT( B1HDLC )     ) printf("HDLC ");
    if ( b1protocol & BIT( B1TRANS )    ) printf("TRANS ");
    if ( b1protocol & BIT( B1V110TRANS )) printf("V110TRANS ");
    if ( b1protocol & BIT( B1V110HDLC ) ) printf("V110HDLC ");
    if ( b1protocol & BIT( B1FAXG3 )    ) printf("FAXG3 ");
    if ( b1protocol & BIT( B1HDLCINV )  ) printf("HDLCINV ");
    if ( b1protocol & BIT( B1HDLC56 )   ) printf("HDLC56 ");
    if ( b1protocol & BIT( B1MODEM )    ) printf("MODEM ");
    if ( b1protocol & BIT( B1MODEMASYNC)) printf("MODEMASYNC ");
    if ( b1protocol & BIT( B1MODEMSYNC )) printf("MODEMSYNC ");
    return 1;
}

int capi2_b2protocol_print ( unsigned long b2protocol ) 
{
    if (!b2protocol) return 0;
    if ( b2protocol & BIT( B2X75             )) printf("X75 ");
    if ( b2protocol & BIT( B2TRANS           )) printf("TRANS ");
    if ( b2protocol & BIT( B2SDLC            )) printf("SDLC ");
    if ( b2protocol & BIT( B2LAPD            )) printf("LAPD ");
    if ( b2protocol & BIT( B2T30             )) printf("T30 ");
    if ( b2protocol & BIT( B2PPP             )) printf("PPP ");
    if ( b2protocol & BIT( B2TRANSERR        )) printf("TRANSERR ");
    if ( b2protocol & BIT( B2MODEM           )) printf("MODEM ");
    if ( b2protocol & BIT( B2X75V42BIS       )) printf("X75V42BIS ");
    if ( b2protocol & BIT( B2V120ASYNC       )) printf("V120ASYNC ");
    if ( b2protocol & BIT( B2V120ASYNCV42BIS )) printf("V120ASYNCV42BIS ");
    if ( b2protocol & BIT( B2V120TRANS       )) printf("V120TRANS ");
    if ( b2protocol & BIT( B2LAPDSAPI        )) printf("LAPDSAPI ");
    /* is not implemented by the brick 
    if ( b2protocol & B2BINTECPPP       ) printf("BINTECPPP ");
    if ( b2protocol & B2BINTECAPPP      ) printf("BINTECAPPP ");
    */
    return 1;
}

int capi2_b3protocol_print ( unsigned long b3protocol ) 
{
    if (!b3protocol) return 0;
    if ( b3protocol & BIT(B3TRANS   )      ) printf("TRANS ");
    if ( b3protocol & BIT(B3T90     )      ) printf("T90 ");
    if ( b3protocol & BIT(B3ISO8208 )      ) printf("ISO8208 ");
    if ( b3protocol & BIT(B3X25DCE  )      ) printf("X25DCE ");
    if ( b3protocol & BIT(B3T30     )      ) printf("T30 ");
    if ( b3protocol & BIT(B3T30EXT  )      ) printf("T30EXT ");
    if ( b3protocol & BIT(B3MODEM   )      ) printf("MODEM ");
    return 1;
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

#if USE_POLL
/*******************************************************************
 *
 *******************************************************************/
int pollset( int fd, int events, int (*func)PROTO((void*)), void *arg ) 
{
    int i;
    struct pollfd *pfp;
    struct pollag *pap;
 
    if (npollfds >= MAX_FD) return -1;
 
    for (i=0, pfp=pollfds, pap=pollags; i < npollfds; ++i, ++pfp, ++pap) {
        if (pfp->fd == fd && pfp->events == events) break;
    }
#if 0
    printf("i=%d npollfds=%d global.conn_calls=%d global.active_calls=%d\n",
	    i,
	    npollfds,
	    global.conn_calls,
	    global.active_calls);
#endif
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
    pap->arg	 = arg;
 
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
    int	pret;
    int i;
 
    while (npollfds > 0) {
        pret = poll(pollfds, npollfds, t);
        switch (pret) {
            case -1:
                if (errno == EINTR) continue;
                if (errno == EAGAIN) continue;
                return -1;
            case 0:
                return 0;
            default:
                pfp = pollfds;
                pap = pollags;
                for (i=0; pret && (i < npollfds); ++i, ++pfp, ++pap) {
                    if (pfp->revents) {
                        (*pap->func)(pap->arg);
                        pfp->revents = 0;
			pret--;
                    }
                }
                return 0;
                break;
        }
    }
    return 0;
}
 
#else	/* USE_POLL */

static int mykernelq = -1;

int kqset( int fd, int events, int (*func)PROTO((void*)), void *arg ) 
{
    struct kevent myevent;
    struct timespec mytimeout;
    struct pollfd *pfp;
    struct pollag *pap;

    bzero(&mytimeout, sizeof(mytimeout));

    if (mykernelq == -1) mykernelq = kqueue();
    if (mykernelq == -1) {
	printf("kqueue() failed: %s\n", strerror(errno));
	abort();
    }

    if (npollfds >= MAX_FD) return -1;

    pfp = pollfds + fd;
    pap = pollags + fd;

    pfp->fd      = fd;
    pfp->events  = events;
    pfp->revents = 0;
    pap->func    = func;
    pap->arg	 = arg;
 
    npollfds ++;

    EV_SET(&myevent, fd, events, EV_ADD, 0, NULL, NULL);
    kevent(mykernelq, &myevent, 1, NULL, 0, &mytimeout);
    return(0);
}

int kqpoll( struct timespec *t )
{
    struct kevent myevent[MAX_FD];
    int myfd = 0;
    struct kevent *myeventp = myevent;
    struct pollag *pap = pollags;
    int i, kret;
    short myfilter;

    while(npollfds > 0) {
	switch(kret = kevent(mykernelq, NULL, 0, myeventp, npollfds, t)) {
	case -1:
	    if (errno == EINTR) continue;
	    if (errno == EAGAIN) continue;
	    return -1;
	    break;
	case 0:
	    return 0;
	    break;
	default:
	    for(i=0; i < kret; i ++, myeventp ++) {
		myfd = myeventp->ident;
		myfilter = myeventp->filter;
		switch(myfilter) {
		case EVFILT_READ:
		    // printf("calling func for fd %d\n", myfd);
		    (*pap[myfd].func)(pap[myfd].arg);
		    break ;
		default:
		    printf("unknown event %d for fd %d\n", myfilter, myfd);
		    break;
		}
	    }
	    return(0);
	}
    }
    return(0);
}
#endif	/* USE_POLL */




/*******************************************************************
 *
 *******************************************************************/
call_t *alloc_call_struct(TCapiAppl_t *pA, unsigned long ident)
{
    call_t *pC;

    pC = (call_t *)calloc(1, sizeof(*pC));

    if (!pC) return NULL;

    pC->pAppl	= pA;
    pC->active	= 0;
    pC->Cip    	= cfg.cip;
    pC->ident  	= ident;
    setBprotocol( pC, 1,
	    cfg.SffFormat, 
	    cfg.stationid,
	    cfg.headline,
	    cfg.speed,
	    cfg.proto); 

    /* attach call to appl call list */
    pC->pNextCall  = pA->pCalls;
    pA->pCalls		= pC;

    /* increment global calls counter */
    pA->active_calls++;

    printf("%c: new call %d of %d: ",
	    cfg.recv ? 'r':'s' ,
	    pA->active_calls, cfg.callperappl);

    return pC;
}

/*******************************************************************
 *
 *******************************************************************/
int dequeue_call_struct( call_t *pC )
{
    call_t **ppC;
    TCapiAppl_t	*pA	= pC->pAppl;

    for (ppC=&(pA->pCalls); *ppC; ppC=&(*ppC)->pNextCall) {
	if (*ppC == pC) {
	    *ppC = pC->pNextCall;
	    return 1;
	}
    }
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
VOID free_call_struct( call_t *pC )
{
    TCapiAppl_t	*pA;

    if (NULL==pC) {
	return;
    }
    pA	= pC->pAppl;
    dequeue_call_struct( pC);
    deattach_call(pC);

    /* increment global calls counter */
    pA->active_calls++;
    free(pC);

    free_appl(pA);
}

/*******************************************************************
 *
 *******************************************************************/
void set_listen (TCapiAppl_t *pA, int onoff )
{
    unsigned long mask;
    int	fd = pA->fd;

    if (!onoff) {
	mask = 0;
	if (cfg.verbose > 3)
	    printf("Dont listen for calls on controller %d for application %d\n",
		    cfg.controller, fd);
    } else {
	mask = 0x30016;
	if (cfg.verbose > 3)
	    printf("Listen for calls on controller %d for application %d\n",
		    cfg.controller, fd);
    }
    if(pA->listen_mask != mask)  {
	//printf("capi2_listen_req: %lx\n", mask ); 
	capi2_listen_req( fd,		
		cfg.controller & 0x7f,	/* Controller */
		0xFFFFFFFF,		/* Info Mask */
		mask,			/* CIP Mask */
		0,			/* CIP Mask2 */
		NULL,			/* Calling party number */
		NULL);			/* Calling party subaddress */
	pA->listen_mask = mask;
    }
}

void print_call_stats(void) {
    TCapiAppl_t *pA	= global.pApplAnchor;
    call_t	*pC	= pA->pCalls;
    time_t	act;
    time_t	ptime;
    int		i=0;

    act=time(0);

    /* In CSV mode print the header */
    if (cfg.csvoutput) {
	printf("\nID;FD;Duration;"
		"TPackets;TBytes;TBytesPerSecond;"
		"RPackets;RBytes;RBytesPerSecond;\n");
    }

    while( pA ) {
	if(NULL==pC) {
	    if (pA->pNextAppl) pA = pA->pNextAppl;
	    else break;
	    pC = pA->pCalls;
	    continue;	/* test again */
	}
	i++;
	ptime = (act - pC->starttime );
	if (cfg.csvoutput) {
	    printf("0x%0lx;%d;%ld;"
		   "%u;%u;%lu;"
		   "%u;%u;%lu;\n",
		   pC->ident,
		   pA->fd,
		   ptime,
		   pC->sentPackets,
		   pC->sentBytes,
		   (pC->sentBytes / ptime),
		   pC->recvdPackets,
		   pC->recvdBytes,
		   (pC->recvdBytes / ptime));
	} else {
	    printf("ID: 0x%0lx FD: %d D: %ld "
		   "TX: %u %u (%lu) "
		   "RX: %u %u (%lu)\n",
		   pC->ident, 
		   pA->fd,
		   ptime,
		   pC->sentPackets,
		   pC->sentBytes,
		   (pC->sentBytes / ptime),
		   pC->recvdPackets,
		   pC->recvdBytes,
		   (pC->recvdBytes / ptime));
	}
	pC = pC->pNextCall;
    }

    /* if no CSV file wanted print some global stats */
    if (!cfg.csvoutput) {
	printf("Got %d calls. Last Call at: %ld State: %s\n",
		i,
		cfg.starttime,
		cfg.recv ? "recv" : "send");
    }
}

void DisconnectAll(void) {
    TCapiAppl_t *pA	= global.pApplAnchor;
    call_t	*pC	= pA->pCalls;

    printf("Disconnecting all calls!\n");
    while( pC && pA ) {
	if(NULL==pC) {
	    pA = pA->pNextAppl;
	    pC = pA->pCalls;
	    continue;	/* test again */
	}
	doDisconnect(pC);
	pC = pC->pNextCall;
    }
}
/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyMessid( TCapiAppl_t *pA, int messid )
{
    call_t *pC;

    for(pC = pA->pCalls; pC; pC = pC->pNextCall) {
	if (pC->messid == messid) return pC;
    }
    if(cfg.verbose >8) {
	printf("getCallbyMessid(0x%x) return == NULL\n", messid);
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyPlci( TCapiAppl_t *pA, unsigned long plci )
{
    call_t *pC;
    unsigned long plci1 = plci;

    plci &= 0xffff;
    for(pC = pA->pCalls; pC; pC = pC->pNextCall) {
	if ((pC->ident & 0xffff) == plci) return pC;
    }
    if(cfg.verbose >8) {
	printf("getCallbyPlci(0x%lx)->(0x%lx) return == NULL\n", plci1, plci);
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
void attach_call(call_t *pC)
{
    TCapiAppl_t		*pA	= pC->pAppl;
    unsigned		ncci	= pC->ident;
    unsigned		idx	= NCCI2IDX(ncci);

    pC->pNextCallHash	= pA->apCallHash[idx];
    pA->apCallHash[idx]	= pC;

}
/*******************************************************************
 *
 *******************************************************************/
void deattach_call(call_t *pC)
{
    TCapiAppl_t		*pA	= pC->pAppl;
    unsigned		ncci	= pC->ident;
    unsigned		idx	= NCCI2IDX(ncci);
    call_t		**ppC;

    for (ppC=&(pA->apCallHash[idx]); *ppC; ppC=&(*ppC)->pNextCallHash) {
	if (*ppC == pC) {
	    *ppC = pC->pNextCall;
	    return;
	}
    }
}
/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyNcci( TCapiAppl_t *pA, unsigned long ncci )
{
    call_t		*pC;
    unsigned		idx	= NCCI2IDX(ncci);

    for(pC = pA->apCallHash[idx]; pC; pC = pC->pNextCallHash) {
	if (pC->ident == ncci) return pC;
    }

    /* not in our hash so search it slowly */
    for(pC = pA->pCalls; pC; pC = pC->pNextCall) {
	if (pC->ident == ncci) return pC;
    }

    if(cfg.verbose >8) {
	printf("getCallbyNcci(0x%lx) return == NULL\n", ncci);
    }
    return NULL;
}

/*******************************************************************
 *
 *******************************************************************/
call_t *getCallbyIdent( TCapiAppl_t *pA, unsigned long ident )
{
    call_t *ptrCall = NULL;

    if(cfg.verbose >9) {
	printf("getCallbyIdent(0x%0lx)\n", ident);
    }
    if (ident >= 0xffff) {
	ptrCall = getCallbyNcci( pA,  ident );
    }
    else if (ident >= 0xff) {
	ptrCall = getCallbyPlci(  pA, ident );
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
struct userdata *setBprotocol( ptrCall, lHighResolution, nFormat,
				    szStationID, szHeaderLine, speed,
				    proto)
call_t *ptrCall;
int  lHighResolution;
int  nFormat;
char *szStationID;
char *szHeaderLine;
int  speed;
int  proto;
{
    struct bprotocol *bprot;
    struct userdata *data;
    struct b3config_faxg3 *b3cfg;
    struct b1config *b1cfg;
    struct b2config *b2cfg;

    bprot = (struct bprotocol *)&ptrCall->Bprotocol;

    if (proto == 1) {
	if (cfg.verbose > 2) printf("setBprotocol using HDLC/X75/TRANS mode\n");
	//PUT_WORD( bprot->b1proto, B1HDLC);
	//PUT_WORD( bprot->b1proto, B1V110TRANS);
	//PUT_WORD( bprot->b1proto, B1V110HDLC);
	PUT_WORD( bprot->b1proto, B1HDLC);
	PUT_WORD( bprot->b2proto, B2X75);
	//PUT_WORD( bprot->b2proto, B2TRANS);
	PUT_WORD( bprot->b3proto, B3TRANS);
	// reset speed to fall throu the rest
	speed = 0;
    } else if (proto == 2) {
	if (cfg.verbose > 2) printf("setBprotocol using FAXG3 mode\n");
	PUT_WORD( bprot->b1proto, B1FAXG3);
	PUT_WORD( bprot->b2proto, B2T30);
	PUT_WORD( bprot->b3proto, B3T30);
    } else if (proto == 3) {
	if (cfg.verbose > 2) printf("setBprotocol using V110TRANS/TRANS/TRANS mode\n");
	PUT_WORD( bprot->b1proto, B1V110TRANS);
	PUT_WORD( bprot->b2proto, B2TRANS);
	PUT_WORD( bprot->b3proto, B3TRANS);
    } else {
	if (cfg.verbose > 2) printf("TRANS mode set: ");
	PUT_WORD( bprot->b1proto, B1TRANS);
	PUT_WORD( bprot->b2proto, B2TRANS);
	PUT_WORD( bprot->b3proto, B3TRANS);
    }
    data = (struct userdata *)&bprot->structlen;

    if (proto == 2 ) {
	if (cfg.verbose > 2) printf("Setting FAX Layer 1 speed to %d\n", speed);
	b1cfg = (struct b1config *) data;
	PUT_WORD( b1cfg->rate, speed);
	PUT_WORD( b1cfg->bpc, 0);
	PUT_WORD( b1cfg->parity, 0);
	PUT_WORD( b1cfg->stopbits, 0);
	b1cfg->length = sizeof( struct b1config) - 1;
    } else if (proto ==3) {
	if (!speed) {
	    speed = 38400; /* default speed is 38400 */
	    printf("V.110 speed defaults to %d.\n", speed);
	}
	if (cfg.verbose > 2) printf("Setting V.110 Layer 1 speed to %d\n", speed);
	b1cfg = (struct b1config *) data;
	PUT_WORD( b1cfg->rate, speed);
	PUT_WORD( b1cfg->bpc, 8);
	PUT_WORD( b1cfg->parity, 0);
	PUT_WORD( b1cfg->stopbits, 0);
	b1cfg->length = sizeof( struct b1config) - 1;
    } else {
	data->length = 0;	/*  b1config  */
    }
    data = (struct userdata *)&data->data[data->length];

    if (proto == 1 ) {
	/* X.75 shit */
	b2cfg = (struct b2config *) data;
	b2cfg->addressA = 3; 
	b2cfg->addressB = 1;
	b2cfg->moduloMode = 8;
	b2cfg->windowSize = 7;
	b2cfg->xidlen = 0;
	b2cfg->length = sizeof( struct b2config) - 1;	
    } else {
	data->length = 0;	/*  b2config  */
    }
    data = (struct userdata *)&data->data[data->length];

    if (proto == 2 ) {
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
    } else {
	data->length = 0;	/*  b3config  */
    }
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
VOID doDisconnect( call_t *pC )
{
    TCapiAppl_t *pA	= pC->pAppl;
    unsigned long plci, ncci;

    ncci = pC->ident;
    plci = pC->ident & 0xffff;

    /* stop sending please */
    pC->doSend = 0;

    switch (pC->state) {
	case B_ConnectPending:
	case Connected:
	    if(cfg.wait4dataconf && pC->NotAcknowledged) {
		if(cfg.verbose>1) {
		    printf("doDisconnect() wait for %d CAPI2_DATAB3_CONF\n", 
			    pC->NotAcknowledged);
		}
		/* don't send a capi2_disconnectb3_req() we have to wait */
		break;
	    }
	    if(pC->NotAcknowledged && cfg.verbose>0) {
		printf("doDisconnect() missing %d CAPI2_DATAB3_CONF\n", 
			pC->NotAcknowledged);
	    }
	    capi2_disconnectb3_req( pA->fd,
				    ncci,
				    NULL);
	    break;

	case D_ConnectPending:
	case B_DisconnectPending:
	    NEWSTATE( pC, D_DisconnectPending);
	    capi2_disconnect_req( pA->fd,
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
VOID SendData( call_t *pC )
{
    TCapiAppl_t		*pA	= pC->pAppl;
    static unsigned short usDataHandle;
    //char		buffer[cfg.datab3Size];
    int			len = cfg.datab3Size;

    if (0== pC->doSend) { return; }

    if(cfg.MaxActivCalls && pC->maxSentPackets ) {
	if ( (pC->sentPackets >= pC->maxSentPackets) ) {
	    if (pC->NotAcknowledged) {
		/* no sending please */
		return;
	    } else {
		doDisconnect(pC);
	    }
	}
    }


    /*
     * Now send next data blk
     */
    if(cfg.verbose > 1) {
	putchar('*');
    }
    sprintf(&send_buffer[usDataHandle%256], "\n#%d-%d ID:%lx P:%d B:%d#\n",
	    usDataHandle,
	    len,
	    pC->ident,
	    pC->sentPackets,
	    pC->sentBytes);

    pC->NotAcknowledged++;		/* count pending confirms */
    pC->sentPackets++;
    usDataHandle++;

    /* statistics */
    pC->sentBytes+=len;

    capi2_datab3_req( pA->fd,
	    pC->ident,
	    &send_buffer[usDataHandle%256],
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
	printf("Sending\n");
    }
    if (!ptrCall->doSend) {
	ptrCall->doSend		= 1;
	ptrCall->wasConnected	= 1;
	// actual call timestamp
	ptrCall->starttime	= time(0);
	// last call timestamp
	cfg.starttime		= ptrCall->starttime;
	// set the maximum send packet count
	ptrCall->maxSentPackets = cfg.maxSentPackets;
	if(cfg.verbose > 1) {
	    putchar('|');
	}
	/*
	 * Fill the window
	 * minimum windowsize is 1
	 */
	cnt = max(1, cfg.usWindowSize);
	while( cnt-- ) {
	    SendData( ptrCall );
	}
	
    } else {
	printf("Already sending!\n");
    }
    attach_call(ptrCall);
    gen_next_call(ptrCall);
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
    ptrCall->recvdPackets++;
    ptrCall->recvdBytes+=datalen;
    if(cfg.verbose > 2) {
	putchar('r');
	if (cfg.verbose > 200) {
	    capi_hexdump( dataptr, datalen, 16, 2);
	}
    }
#if 0
    if (cfg.MaxActivCalls) {
	printf("messid=%x len=%d\n",messid,datalen);
	    capi_hexdump( dataptr, datalen, 16, 2);
    }
#endif
    return 1;	/*  response */

    /* to make compiler happy */
    messid = 0;
    handle = 0;
    flags  = 0;
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
    }
    SendData( ptrCall );		/* Send the next one	*/

    return;
    messid = 0;
    handle = 0;
}

/*******************************************************************
 *
 *******************************************************************/
VOID
handleInfoInd( call_t *ptrCall, unsigned short infonumber, struct userdata	*data )
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
    return;
    ptrCall = NULL;

}

int gen_call(TCapiAppl_t *pA)
{
    call_t 	*pC;

    if (pA->active_calls >= cfg.callperappl) {
	printf("calls per appl exceeded %d\n", pA->active_calls);
	return 0;
    }

    pC = alloc_call_struct(pA, cfg.controller);
    if (!pC) {
	fprintf( stderr, "alloc_call failed!\n");
	return 1;
    }
    pC->active = 1;
    setCalledPartyNumber(      pC, cfg.rmttelno);
    setCallingPartyNumber(     pC, cfg.loctelno, 1);
    setCalledPartySubaddress(  pC, cfg.rmtsubaddr);
    setCallingPartySubaddress( pC, cfg.locsubaddr);
    printf("Dialing: ");
    pC->messid = capi2_connect_req( pA->fd,
	    pC->ident & 0x7f,
	    pC->Cip,
	    &pC->CalledPartyNumber,
	    &pC->CallingPartyNumber,
	    &pC->CalledPartySubaddress,
	    &pC->CallingPartySubaddress,
	    &pC->Bprotocol,
	    NULL, NULL, NULL,
	    NULL);
    if(cfg.verbose>8) {
	printf("messid from capi2_connect_req() == 0x%x\n", pC->messid);
    }
    NEWSTATE( pC, D_ConnectPending);
    return 0;
}

/*******************************************************************
 *
 *******************************************************************/
VOID
handleDisconnectB3Ind( call_t *ptrCall,
	unsigned short nReason,
	struct userdata *ncpi )
{
    int			delta_time;

    cfg.endtime = time(0);
    delta_time  = cfg.endtime - cfg.starttime;

    printf("\nDisconnect for call 0x%0lx \n", ptrCall->ident);

    if (nReason) printf("\tB3_Reason (0x%04x)\n", nReason);

    if(ptrCall->NotAcknowledged && cfg.verbose>0) {
	printf("handleDisconnectB3Ind() missing %d CAPI2_DATAB3_CONF\n", 
		ptrCall->NotAcknowledged);
    }
    return;
    ncpi = NULL;
}

/*******************************************************************
 *
 *******************************************************************/
VOID handleDisconnectInd( call_t *ptrCall, unsigned short nReason )
{
    printf("ISDN D-channel disconnect, Reason (%04x)\n", nReason);
    printf("\n");
    return;
    ptrCall = NULL;
}


/*******************************************************************
 *
 *******************************************************************/
int capi_event(void *arg)
{
    TCapiAppl_t *pA	= arg;
    int			fd	= pA->fd;
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
	case CAPI2_E_MSG_QUEUE_EMPTY:	 	return 0; break;
	case CAPI2_E_INTERNAL_BUSY_CONDITION:	return 0; break;
	case CAPI2_E_OS_RESOURCE_ERROR:		
	    printf("capi_event error cause: %d\n", lcause);
	    // stop all calls for now
	    global.endloop = 1;
	    return -1;
	    break;
	default:				break;
    }

    info    = getCapiInfo(capi);
    if (info) capi2_perror("\nCAPI Info", info);

    /*
     *  get the call structure
     */
    ident   = GET_c2IDENT(capi);
    ptrCall = getCallbyIdent( pA, ident);

    switch (GET_PRIMTYPE(capi)) {
	case CAPI2_CONNECT_IND:
	    printf("Ring ring: ");
	    /*  handleConnectInd( );  */
	    ptrCall = alloc_call_struct( pA, ident);
	    check_appl(pA);	/* verify current situation */
	    if (ptrCall) {
		    capi2_connect_resp( fd, 
			    GET_MESSID(capi),
			    ident,        
			    0, 			/* accept */
			    &ptrCall->Bprotocol,
			    NULL,  			/* cad */
			    NULL,  			/* csa */
			    NULL,  			/* llc */
			    NULL); 			/* add */
		NEWSTATE( ptrCall, D_ConnectPending);
	    } else {
		printf("ERROR: could not allocate call\n");
	    }
	    break;
	case CAPI2_LISTEN_CONF:
	    break;
	
	case CAPI2_CONNECT_CONF:
	    ptrCall = getCallbyMessid(pA, GET_MESSID(capi));
	    if (ptrCall) {
		ptrCall->ident = ident;
		printf("Connected: ");
		if (info) {	/* CONNECT_REQ failed -> end program */ 
		    //mypolldel( ptrCall->capi_fd );
		    free_call_struct( ptrCall );
		    // global.endloop = 1;
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
	    ptrCall = getCallbyPlci(pA, ident & 0xffff );
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
	case CAPI2_CONNECTB3_IND:
	    if (cfg.verbose > 3) printf("CAPI2_CONNECTB3_IND\n");
	    ptrCall = getCallbyPlci( pA, ident );
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

		//mypolldel( ptrCall->capi_fd );
		free_call_struct( ptrCall );
	    }
	    // global.endloop = 1;
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
    fprintf( stderr, "\t-l: local telefone number\n");
    fprintf( stderr, "\t-y: max calls per RCAPI TCP connection\n");
    fprintf( stderr, "\t-p: protocol: 0 (TRANS/TRANS/TRANS), 1 (HDLC,X.75/TRANS), 2 (FAXG3)\n");
    fprintf( stderr, "\t-r: run in listen mode (answer calls)\n");
    fprintf( stderr, "\t-s: specify calls to dialup on startup\n");
    fprintf( stderr, "\t-h: specify user part of fax header line\n");
    fprintf( stderr, "\t-i: specify local station identification\n");
    fprintf( stderr, "\t-a: specify maximum FAX transfer speed in bps\n");
    fprintf( stderr, "\t-c: specify ISDN controller to use [1..n]\n");
    fprintf( stderr, "\t-C: specify CIP\n");
    fprintf( stderr, "\t-B: specify DATAB3 block size\n");
    fprintf( stderr, "\t-M: specify maximum packets to send until disconnect\n");
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
VOID free_appl(TCapiAppl_t * pA)
{
    TCapiAppl_t		**ppA;

    if (pA->conn_calls) {
	return;
    } else {
	printf("Freeing appl %p\n", pA);
    }
	
   
    /* deque from global.pApplAnchor  */
    for(ppA = &(global.pApplAnchor); *ppA; ppA = &(*ppA)->pNextAppl ){
	if( *ppA == pA) {
	    *ppA = pA->pNextAppl;
	    global.appl_cnt--;
	    break;
	}
    }
    /* kill calls */

    /* kill poll  */
    
    /* release from capi */
    if(pA->fd) {
	printf("Closing appl fd %d\n", pA->fd);
	mypolldel( pA->fd );
	capi2_release(pA->fd);
	pA->fd = 0;
    }
    free(pA);

    if (!global.appl_cnt) {
	printf("Quiting Applicatin, no more calls there\n");
	global.endloop = 1;
    }
}

/*******************************************************************
 *
 *******************************************************************/
TCapiAppl_t *alloc_appl(VOID)
{
    TCapiAppl_t	*	pA = calloc(1, sizeof(*pA));

    if(NULL==pA) return(NULL);

    
    for(;;) { 
	if(cfg.verbose > 2){
	    putchar('R');
	    fflush(stdout);
	}
	
	pA->fd = capi2_register( REG_CAPI_MSG_SIZE,
				 REG_CAPI_LEVEL3CNT,
				 cfg.usWindowSize,
				 cfg.datab3Size,
				 NULL);
	
	if(pA->fd >0 ) break;
	fprintf( stderr, "capi2_register failed! retry in %d seconds\n",
		 REGISTER_FAIL_TIMER     );
	sleep(REGISTER_FAIL_TIMER);
    }

    printf("new capi %d: ", pA->fd);

    pA->pNextAppl	= global.pApplAnchor;
    global.pApplAnchor	= pA;
    global.appl_cnt++;
#if USE_POLL
    pollset( pA->fd, POLLIN, capi_event, pA);
#else
    kqset( pA->fd, EVFILT_READ, capi_event, pA);
#endif

    /* if in listen mode, switch on listen on CAPi conn */
    if (cfg.recv) {
	if (cfg.verbose > 3) printf("Going into LISTEN mode\n");
	set_listen(pA, 1);
    } 

    if (cfg.verbose > 2 ) { 
	printf("alloc_appl: %c#%d\n", cfg.recv?'r':'s', global.appl_cnt);
    }
    return pA;
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

    /* not buffering */
    setbuf(stdout, 0);
    setbuf(stderr, 0);

    return 0;
    argc = 0;
}

/*******************************************************************
 *
 *******************************************************************/
int main(argc, argv)
int argc;
char **argv;
{ 
    TCapiAppl_t *pA;
    int		i;
    int		max_controller;
    unsigned long controller_mask = 0;
    struct capi_getprofile profile;
    size_t	sendlen;
    char 	c_manufacturer[256];
    unsigned long c_version = 0;
    char	c_serial[100];
    struct timespec timeout;


    extern int		optind;
    extern char 	*optarg;

    init_program(argc, argv);
    bzero(&timeout, sizeof(timeout));

    /*
     * get options
     */
    while ((i = getopt( argc, argv, CMD_LINE_ARGS )) != EOF) switch (i) {
	case 'h': strncpy( cfg.headline,  optarg, sizeof(cfg.headline));  break;
	case 'i': strncpy( cfg.stationid, optarg, sizeof(cfg.stationid)); break;
	case 'l': strncpy( cfg.loctelno,  optarg, sizeof(cfg.loctelno));  break;
	case 'c': cfg.controller    = atoi(optarg);			  break;
	case 'y': cfg.callperappl   = atoi(optarg);			  break;
	case 'a': cfg.speed         = atoi(optarg);		    	  break;
	case 'p': cfg.proto         = atoi(optarg);		    	  break;
	case 'r': cfg.recv          = 1;			  	  break;
	case 's': cfg.MaxActivCalls = atoi(optarg);		  	  break;
	case 'M': cfg.maxSentPackets= atoi(optarg);			  break;
	case 'q': cfg.verbose       = 0;			  	  break;
	case 'v': cfg.verbose       = atoi(optarg);		    	  break;
	case 'C': cfg.cip           = atoi(optarg);			  break;
	case 'w': cfg.usWindowSize  = atoi(optarg);			  break;
	case 'B': cfg.datab3Size    = atoi(optarg);			  break;
	case 'W': cfg.wait4dataconf = atoi(optarg);			  break;
	case 'S': cfg.csvoutput	    = 1;				  break;
	case 't': strncpy( cfg.cmd,  optarg, sizeof(cfg.cmd));
		  /* fall thru is ok */
	case 'T': cfg.pcmd = cfg.cmd;			 		  break;
	default:  usage(); exit(1); 
    }

    if (optind < argc) {
	strncpy( cfg.rmttelno, argv[optind++], sizeof(cfg.rmttelno));
    } else {
	if (!cfg.recv) {
	    usage();
	    exit(1);
	}
    }

    // fill the send buffer
    for (sendlen = 0; sendlen < sizeof(send_buffer); sendlen++) {
	send_buffer[sendlen] = sendlen;
    }

    /* some information about the capi */
    if (capi2_get_manufacturer( c_manufacturer) < 0 ) {
	printf("Could not contact BRICK!\n");
	exit(1);
    }
    printf("Manufacturer: %s\n", c_manufacturer);
    capi2_get_version( &c_version );
    printf("Version     : %lu\n", c_version );
    capi2_get_serial( c_serial );
    printf("Serial      : %s\n", c_serial );
    /* now also get some CAPI1.1 manufacturer info
     * normally the BOSS version string is in it.
     * we reuse c_manufacturer */
    capi_get_version( c_manufacturer );
    printf("BOSS Version: %s\n", c_manufacturer );


    /* start inquiering the brick controllers */
    memset( &profile, 0, sizeof(profile));
    capi2_get_profile( 0, &profile);
    max_controller = profile.ncontrl;

    printf("Available ISDN controllers: %d\n", max_controller);

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

    /* some info about the controller */
    fprintf( stderr, "Controller: %4d\n", (int) profile.ncontrl); 
    fprintf( stderr, "Channels  : %4d \n", (int) profile.nchannel); 
    fprintf( stderr, "Options   : "); 
    capi2_options_print(profile.options); printf("\n");
    fprintf( stderr, "B1Protocol: ");
    capi2_b1protocol_print(profile.b1protocol); printf("\n");
    fprintf( stderr, "B2Protocol: ");
    capi2_b2protocol_print(profile.b2protocol); printf("\n");
    fprintf( stderr, "B3Protocol: ");
    capi2_b3protocol_print(profile.b3protocol); printf("\n");

#if 0	/* debug printf */
    if ( !(profile.b1protocol & (1 << 4)) ||
	 !(profile.b2protocol & (1 << 4)) ||
	 !(profile.b3protocol & (1 << 4))) {
	fprintf( stderr, "Controller %d: does not support FAX G3\n",
						cfg.controller);
	return 1;
    }
#endif

    /*
     * do register 
     */
    pA = alloc_appl();
    if (NULL==pA) {
	fprintf( stderr, "capi2_register failed!\n");
	return -1;
    }

    /* generate first call to startup dialing if we in send mode */
    fprintf( stderr, "going to start next call\n");
    if(cfg.recv) {
	check_appl(NULL);
    }
    if(cfg.MaxActivCalls) {
	gen_next_call(NULL);
    }


    while (!global.endloop ) {		// XXX: introduce daemon
#if USE_POLL
	if (pollloopt(-1) < 0) break;
#else
	/* timeout still unused */
	if (kqpoll(NULL) < 0) break;
#endif
    }
    exit(1);
}

