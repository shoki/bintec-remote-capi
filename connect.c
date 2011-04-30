/******************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: connect.c,v $
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
 *      -
 *****************************************************************************/

#ifdef USE_RCSID_C
    static const char _rcsid_connect_c[] __UNUSED = "$Id: connect.c,v 1.1 2002/09/17 20:45:46 shoki Exp $";
#endif

#include "capiconf.h"


#ifdef BOSS
#include <sys/ld.h>
char const app[] = "connect";
unsigned const stksz = 0;
#endif

#define bzero(a,b)	memset((a), 0, (b))


FILE *filefp = NULL;


int api_plci[2] = {-1, -1};
int api_ncci[2] = {-1, -1};
int api_eaz[2] = { '1', '1'};
int api_si[2] = { 1, 1};
int api_add[2] = { 1, 1};

int api_ctrl = 0;

char caller[2]={1,1};

int capifd;
int call = 0;

struct telno telno[2] = { 
	    { 4, 0x81, "180" },
	    { 4, 0x81, "180" }
};


#ifdef BOSS
# define bzero(a,b)	memset((a), 0, (b))
# define raw() 		ioctl( 0, LDSETMODE, ioctl(0, LDGETMODE) | LDM_RAW)
# define cooked()	ioctl( 0, LDSETMODE, ioctl(0, LDGETMODE) & ~LDM_RAW) 
# define input(a,b)		\
	{			\
	    fflush(stdin);	\
	    fflush(stdout);	\
	    cooked();		\
	    fgets((a), (b), stdin);	\
	    raw();		\
	}
#else	/* not BOSS */
# define raw()
# define cooked()
# define input(a,b)  	scanf("%s", (a));
#endif	/* BOSS */


#ifdef __STDC__
int	main();
int 	api_start();
void	key_event( int, int);
void 	api_event( int, int);
int	get_call(int);
int	get_link(int);
int	get_newcall();
#else
int	main();
int 	api_start();
void	key_event();
void	api_event();
int	get_call();
int	get_link();
int	get_newcall();
#endif



struct pollag {
    void (*func)();
};

#define MAXFD 64
static int npollfds;
static struct pollfd pollfds[MAXFD];
static struct pollag pollags[MAXFD];

int pollset(fd, events, func)
int fd;
int events;
void (*func)();
{
    int i;
    struct pollfd *pfp;
    struct pollag *pap;

    if (npollfds >= MAXFD) return -1;

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

int polldel(fd)
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
			(*pap->func)(pfp->fd, pfp->revents);
			pfp->revents = 0;
		    }
		}
		return 0;
		break;
	}
    }
    return 0;
}




int main()
{
    api_start();
    return 0;
}

enum api_states { IDLE, WAITING, CONNECTED } state[2] = { IDLE, IDLE };

static char ncpd_t30[] = { 8, 0, 0, 5, 0, 2, '1', '2', 0};

void key_event( fd, event )
int fd;
int event;
{
    int i;
    char c;
    int eazmask;
    char buf[100];
    struct telno condigit;


    if (read( fd, (char *)&c, 1) == -1) {
	perror("key_event");
	return;
    }

    c = toupper(c);
    
    switch (c) {
	case 'S':
	    if (state[call] != IDLE) {
		printf("\nWRONG STATE(%d) -> CONNECT_REQ NOT ALLOWED\n", call);
	    } else {
		printf("CONNECT_REQ(%d) ->T: ", call);
		if (telno[call].length) { 
		    for (i=0; i<telno[call].length-1; i++)
			printf("%c", telno[call].no[i]);
		}
		printf(" E: %c SI: %d Add %d\n", api_eaz[call], 
						 api_si[call], 
						 api_add[call]);
		capi_connect_req( capifd, api_ctrl, 0x83, 
				    (CAPI_INFOMASK & 0xff), 
					api_si[call], 
					api_add[call], 
					api_eaz[call], 
					&telno[call], NULL);
		state[call]  = CONNECTED;
		caller[call] = 1;
	    }
	    break;
	 case '1': case '2': case '3':
	 case '4': case '5': case '6':
	 case '7': case '8': case '9':
	 case '*': case '0': case '#':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> DIAL NOT ALLOWED\n", call);
	    } else {
		printf("CONNECTINFO_REQ(%d)-> D:%c\n", call, c);

		condigit.length = 2;
		condigit.type   = 0x81;
		condigit.no[0]  = c;
		capi_connectinfo_req( capifd, api_plci[call], &condigit);
	    }
	    break;
	 case 'C':
	    if (state[call] != WAITING) {
		printf("\nWRONG STATE(%d) -> CONNECT_RESP NOT ALLOWED\n", call);
	    } else {
		printf("CONNECT_RESP(%d) ->\n", call);

		capi_selectb2_req( capifd, api_plci[call], L2TXONLY, NULL);
		capi_selectb3_req( capifd, api_plci[call], L3TRANS, NULL);
		capi_listenb3_req( capifd, api_plci[call]);
		capi_connect_resp( capifd, 0, api_plci[call], 0);

		state[call] = CONNECTED;
	    }
	    break;
	 case 'L':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> LISTENB3 NOT ALLOWED\n",
							 call);
	    } else {
	        capi_listenb3_req( capifd, api_plci[call]);
	    }
	    break;
	 case 'N':
	    if (state[call] != WAITING) {
		printf("\nWRONG STATE(%d) -> CONNECT_RESP REJECT NOT ALLOWED\n"
							, call);
	    } else {
		printf("CONNECT_RESP REJECT(%d) ->\n", call);

		capi_connect_resp( capifd, 0, api_plci[call], 1);
		state[call] = IDLE;
	    }
	    break;
	 case 'D':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> DISCONNECT NOT ALLOWED\n",
							 call);
	    } else {
		printf("DISCONNECT_REQ(%d) ->\n", call);
		capi_disconnect_req( capifd, api_plci[call], 0);
		state[call] = IDLE;
	    }
	    break;
	case 'R':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> RESETB3_REQ NOT ALLOWED\n", call);
	    } else {
		printf("RESETB3_REQ(%d) ->\n", call);
		capi_resetb3_req( capifd, api_ncci[call]);
	    }
	    break;
	case 'U':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> RESETB3_RESP NOT ALLOWED\n", call);
	    } else {
		printf("RESETB3_RESP(%d) ->\n", call);
		capi_resetb3_resp( capifd, 0, api_ncci[call]);
	    }
	    break;
#if 0
	case 'F':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> DATAB3_REQ NOT ALLOWED\n", call);
	    } else {
		printf("DATAB3_REQ(%d) ->\n", call);
		capi_datab3_req( capifd,api_ncci[call],
					(char *)"test Data", 9, 0, 0);
	    }
	    break;
#endif
	case 'V':
            if (state[call] != CONNECTED) {
                printf("\nWRONG STATE(%d) -> DATAB3_RESP NOT ALLOWED\n", call);
	    } else {
                capi_datab3_resp( capifd, 0, api_ncci[call], 0);
		printf("DATAB3_RESP(%d)\n", call);
		capi_datab3_resp( capifd, 0, api_ncci[call], 1);
		printf("DATAB3_RESP(%d)\n", call);
		capi_datab3_resp( capifd, 0, api_ncci[call], 2);
		printf("DATAB3_RESP(%d)\n", call);
	    }
	    break;
	case 'X':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> DISC B3 NOT ALLOWED\n", call);
	    } else {
		printf("DISCONNECTB3_REQ(%d)\n", call);
		capi_disconnectb3_req( capifd, api_ncci[call], NULL);
	    }
	    break;
	 case 'Z':
	    if (state[call] != CONNECTED) {
		printf("\nWRONG STATE(%d) -> CONNECTB3_REQ NOT ALLOWED\n",call);
	    } else {
		printf("CONNECTB3_REQ(%d)\n", call);
		capi_connectb3_req( capifd, api_plci[call], NULL);
	    }
	    break;
	 case 'T':
	    if (state[call] != IDLE) {
		printf("\nWRONG STATE(%d) -> TELNO NOT ALLOWED\n", 
							call);
	    } else {
		printf("Telefonnumber(%d): ", call);
		input( telno[call].no, 32);
		telno[call].length = strlen(telno[call].no)+1;
		printf("New Telefonnumber(%d): <%s>\n", 
					call, telno[call].no);
	    }
	    break;
	 case 'E':
	    if (state[call] != IDLE) {
		printf("\nWRONG STATE(%d) -> EAZ_IND NOT ALLOWED\n", 
						call);
	    } else {
		printf("eaz ind(%d) valid (0-9,-) ", call);
		input( buf, 100);
		printf("New eaz ind(%d): <%c>\n", call, buf[0]);
		if (buf[0] == '-') {
		    api_eaz[call] = 0;
		    eazmask = CAPI_EAZMASK;
		} else {
		    api_eaz[call] = buf[0];
		    eazmask = (1 << (api_eaz[call]-'0')) | 1;
		}
		capi_listen_req( capifd, 0, 
				(CAPI_INFOMASK & 0xff),
				eazmask, (1 << api_si[call]));
	    }
	    break;
	 case 'I':
	    if (state[call] != IDLE) {
		printf("\nWRONG STATE(%d) -> SI NOT ALLOWED\n", call);
	    } else {
		printf("si(%d) (1-15)  ", call);
		input( buf, 100);
		printf("New si(%d): <%d>\n", call, atoi(buf));
		api_si[call] = atoi(buf);
		if (api_eaz[call] == 0) {
		    eazmask = 0xffff;
		} else {
		    eazmask = 1 << (api_eaz[call]-'0');
		}
		capi_listen_req( capifd, 0, 
				(CAPI_INFOMASK & 0xff),
				eazmask, (1 << api_si[call]));
	    }
	    break;
	 case 'A':
	    if (state[call] != IDLE) {
		printf("\nWRONG STATE(%d) -> Add NOT ALLOWED\n", call);
	    } else {
		printf("addinfo(%d) ", call);
		input( buf, 100);
		printf("New addinfo(%d): <%d>\n", call, atoi(buf));
		api_add[call] = atoi(buf);
	    }
	    break;
	 case 'B':
	    printf("Board(%d) ", call);
	    input( buf, 100);
	    printf("New Controller(%d): %d\n", call, atoi(buf));
	    api_ctrl = atoi(buf);
	    break;
	case 'Q':
	    close(capifd);
	    exit(0);
	    break;
    }
    return;
}


void api_event(fd, events)
int fd, events;
{
    struct telno *telno;
    char *ptr;
    int ev, i, act, datalen;
    union CAPI_primitives proto;
    union CAPI_primitives *cmsg;
    unsigned char data[4096];

loop:
    ev = capi_get_message( fd, &cmsg, &proto, data, sizeof(data));
    switch (ev) {
	case CAPI_E_NOMSG:			return;
	case CAPI_E_CONTROLLERFAILED:	exit(1); return;
    	case 0: 		break;
	default:		return;
    }

    switch (GET_PRIMTYPE(cmsg)) {
	case CAPI_LISTEN_CONF:
	    break;

	case CAPI_CONNECT_CONF:
	    act = get_newcall();
	    if (act != -1) {
		api_plci[act] = GET_PLCI(cmsg);
	    }
	    capi_selectb2_req( capifd, GET_PLCI(cmsg), L2TXONLY, 0);
	    capi_selectb3_req( capifd, GET_PLCI(cmsg), L3TRANS, 0);
	    break;

	case CAPI_CONNECT_IND:
	    act = get_newcall();
	    if (act != -1) {
		api_plci[act] = GET_PLCI(cmsg);
		caller[act]   = 0;

		telno = (struct telno *)&cmsg->connect_ind.telnolen;

		printf("-> CONNECT_IND(%d) T:", act);
		if (telno->length) {
		    for (i=0; i<telno->length-1; i++) 
			printf("%c", telno->no[i]);
		}
		printf(" E: %c SI: %d Add: %d\n", cmsg->connect_ind.dst_eaz, 
						  cmsg->connect_ind.dst_service,
						  cmsg->connect_ind.dst_addinfo);

		state[act] = WAITING;
	    }
	    break;

	case CAPI_CONNECTACTIVE_IND:
	    capi_connectactive_resp( capifd, GET_MESSID(cmsg), GET_PLCI(cmsg));

	    act = get_call( GET_PLCI(cmsg));
	    if (act != -1) {
		printf("-> CONNECTACTIVE_IND(%d)\n", act);
		if (caller[act]) {
		    printf("-> CONNECTB3_REQ(%d)\n", act);
		    capi_connectb3_req( capifd, api_plci[act], NULL);
		}
	    }
	    break;

	case CAPI_INFO_IND:
	    capi_info_resp( capifd, GET_MESSID(cmsg), GET_PLCI(cmsg) );
	    act = get_call( GET_PLCI(cmsg));
	    if (act != -1) {
		printf("-> INFO_IND(%d) TYPE %04x", act, 	
					GET_WORD(cmsg->info_ind.info_number));
		for (i=0, ptr = (char *)&cmsg->info_ind.infolen+1;
		     i < cmsg->info_ind.infolen; ptr++, i++) {
		     if (isprint(*ptr)) {
			 printf("%c", *ptr);
		     } else {
			 printf(".");
		     }
		}
		printf("\n");
	    }
	    break;

	case CAPI_DATA_IND:
	    capi_data_resp(capifd, GET_MESSID(cmsg), GET_PLCI(cmsg) );
	    act = get_call( GET_PLCI(cmsg) );
	    if (act != -1) {
		printf("-> DATA_IND(%d)\n", act);
	    }
	    break;

	case CAPI_DISCONNECT_IND:
	    capi_disconnect_resp( capifd, GET_MESSID(cmsg), GET_PLCI(cmsg));
	    act = get_call( GET_PLCI(cmsg));
	    if (act != -1) {
		printf("-> DISCONNECT_IND(%d) cause: 0x%x\n", act, 
					    GET_WORD(cmsg->disconnect_ind.info));
		state[act] = IDLE;
		api_plci[act] = -1;
	    }
	    break;

	case CAPI_CONNECTB3_CONF:
	    act = get_call( GET_WORD(cmsg->connectb3_conf.plci));
	    if (act != -1) {
		api_ncci[act] = GET_WORD(cmsg->connectb3_conf.ncci);
	    }
	    break;

	case CAPI_CONNECTB3_IND:
	    capi_connectb3_resp( capifd, GET_MESSID(cmsg), 
				 GET_WORD(cmsg->connectb3_ind.ncci), 0, NULL);

	    act = get_call( GET_WORD(cmsg->connectb3_ind.plci));
	    if (act != -1) {
		printf("-> CONNECTB3_IND(%d)\n", act);
		api_ncci[act] = GET_WORD(cmsg->connectb3_ind.ncci);
	    }
	    break;

	case CAPI_DISCONNECTB3_IND:
	    capi_disconnectb3_resp( capifd, GET_MESSID(cmsg), GET_NCCI(cmsg));

	    act = get_link( GET_NCCI(cmsg));
	    if (act != -1) {
		printf("-> DISCONNECTB3_IND(%d)\n", act);
		api_ncci[act] = -1;
	    }
	    break;

	case CAPI_CONNECTB3ACTIVE_IND:
	    capi_connectb3active_resp( capifd, GET_MESSID(cmsg), GET_NCCI(cmsg));
	    act = get_link( GET_NCCI(cmsg));

	    if (act != -1) {
		printf("-> CONNECTB3ACTIVE_IND(%d)\n", act);
	    }
#ifndef BOSS
	    if ((filefp = fopen("lola", "r"))) {
		datalen = fread( data, 1, 512, filefp);
		if (datalen > 0) {
		    capi_datab3_req( capifd, 
				     GET_NCCI(cmsg), 
				     data, 
				     datalen,
				     0, 0);
		}
	    }
#endif
	    break;

	case CAPI_RESETB3_CONF:
	    printf("-> RESETB3_CONF(%d)\n", act);
	    break;

	case CAPI_RESETB3_IND:
	    act = get_link( GET_NCCI(cmsg));
	    if (act != -1) {
		printf("-> RESETB3_IND(%d)\n", act);
	    }
	    break;

	case CAPI_DATAB3_IND:
	    act = get_link( GET_NCCI(cmsg));
	    if (act != -1) {
		printf("-> DATAB3_IND(%d) %d BYTES\n", act, 
					    GET_WORD(cmsg->datab3_ind.datalen));
	    }
	    break;

	case CAPI_DATAB3_CONF:
	    act = get_link( GET_NCCI(cmsg));
	    if (act != -1) {
		printf("-> DATAB3_CONF(%d)\n", act);
	    }
#ifndef BOSS
	    if (filefp) {
		datalen = fread( data, 1, 512, filefp);
		if (datalen > 0) {
		    capi_datab3_req( capifd, 
				     GET_NCCI(cmsg), 
				     data, 
				     datalen,
				     0, 0);
		}
	    }
#endif
	    break;

	case CAPI_HANDSET_IND:
	    printf("-> HANDSET_IND '%c'\n", cmsg->handset_ind.state);
	    capi_handset_resp( capifd, GET_MESSID(cmsg), GET_PLCI(cmsg));
	    switch (cmsg->handset_ind.state) {
		case '+':
		   act = get_newcall();
		   state[act] = CONNECTED;
		   if (act != -1) {
		       api_plci[act] = GET_PLCI(cmsg);
		       caller[act]   = 0;
		   }
		   break;
		case '-':
		   break;
	    }
	    break;
	case CAPI_DTMF_IND:
	    printf("-> DTMF_IND ");
	    for (i=0; i < cmsg->dtmf_ind.dtmflen; i++) {
		printf( "%c ", *((char *)&cmsg->dtmf_ind.dtmflen + (i+1)));
	    }
	    printf("\n");
	    capi_dtmf_resp( capifd, GET_MESSID(cmsg), GET_PLCI(cmsg));
	    break;
    }
    goto loop;
}



int get_call( plci )
int plci;
{
    int i;
    for (i=0; i < 2; i++) {
	if (api_plci[i] == plci) {
	    return i;
	}
    }
    return -1;
}

int get_link( ncci )
int ncci;
{
    int i;
    for (i=0; i < 2; i++) {
	if (api_ncci[i] == ncci) {
	    return i;
	}
    }
    return -1;
}

int get_newcall( )
{
    int i;
    for (i=0; i < 2; i++) {
	if (api_plci[i] == -1) {
	    return i;
	}
    }
    return -1;
}





int api_start()
{

    char apimsgbuffer[200];

    capi_get_manufacturer( apimsgbuffer);
    printf("MANUFACTURER: %s\n", apimsgbuffer);
    capi_get_version( apimsgbuffer);
    printf("VERSION     : %s\n", apimsgbuffer);
    capi_get_serial( apimsgbuffer);
    printf("SERIAL      : %s\n", apimsgbuffer);

    if ((capifd = capi_register( 50, 2, 4, 2048, NULL )) == -1) {
	return -1;
    }

/*
    if ((capifd = capi_open(inet_addr("192.54.53.108"))) == -1) {
	perror("capi_open");
    }
*/

    capi_listen_req( capifd,api_ctrl, 
		     (CAPI_INFOMASK & 0xff),
		     CAPI_EAZMASK, CAPI_SIMASK);

    printf("\n");
    printf("keys -> T=TELNO, S=SETUP, C=CONNECT, N=REJECT, D=DISCONNECT\n");
    printf("keys -> E=IND EAZ, I=SI A=Add, B=Board, Q=QUIT\n");
    printf("keys -> R=RESET_REQ, U=RESET_RESP, F=DATA_REQ, V=DATA_RESP\n");
    printf("keys -> Z=B3CONN_REQ, X=B3DISC L=LISTENB3\n");

    pollset( 0, POLLIN, key_event);
    pollset( capifd, POLLIN | POLLPRI, api_event);

    raw();
    for(;;) {
	if (pollloopt(-1) < 0) break;
    }
    cooked();

    return 0;
}

