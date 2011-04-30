/************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: capitrace.c,v $
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

#include "capidump.h"


#define M_OPEN                  0xf1
#define M_CLOSE                 0xf2
#define M_STAT                  0xf3
 
#ifdef BOSS
const char app[]="capitrace";
const int stksz=32768;
#endif

#define MSGBUFSIZE	1024

#ifndef PROTO
#ifdef __STDC__
#define PROTO(x)	x
#else
#define PROTO(x)	()
#endif
#endif

int	usage PROTO(( char *));
int	main PROTO(( int, char **));


VOID dump_msg PROTO(( unsigned char * buffer, int len, int flag));

 

/************************************************************************/
/*									*/
/*	usage								*/
/*									*/
/************************************************************************/
int usage(name)
char *	name;
{
    fprintf(stderr, "usage: %s [-hsl]\n", name);
    fprintf(stderr, "	-h	hexadecimal output\n");
    fprintf(stderr, "	-s	short output\n");
    fprintf(stderr, "	-l	long output\n");
    fprintf(stderr, "	Environment variables:\n");
    fprintf(stderr, 
	"\t  CAPI_HOST\tspecify trace host (BRICK's name or IP address)\n");
    fprintf(stderr, 
	"\t  CAPI_PORT\tspecify trace port (default: TCP port 6000)\n");
    fprintf(stderr, "\n");

    return 1;
}

/**********************************************************************/
/*                                                                    */
/*        dump_msg                                                    */
/*                                                                    */
/**********************************************************************/
VOID dump_msg( buffer, len, flag)
    unsigned char * buffer;
    int len, flag;
{
    capitrace_t	*tracemsg;
    union CAPI_primitives *capi;

    tracemsg = (capitrace_t *)buffer;
    capi = (union CAPI_primitives *)(buffer + sizeof(capitrace_t));

    len -= sizeof(capitrace_t);

    tracemsg->timestamp = ntohl(tracemsg->timestamp);
    tracemsg->inout     = ntohl(tracemsg->inout);
    tracemsg->type      = ntohl(tracemsg->type);
    tracemsg->seqcnt    = ntohl(tracemsg->seqcnt);

    dump_capimsg( capi, (unsigned long) flag, tracemsg->inout,
			tracemsg->timestamp, tracemsg->seqcnt);
    fflush( stdout);
}


/************************************************************************/
/*									*/
/*	main								*/
/*									*/
/************************************************************************/
int main(argc, argv)
    int 	argc;
    char **	argv;
{
    int rn, len, mode, flag, i, fd;
    unsigned cnt = 0;
    struct pollfd pollfd;
    unsigned char buffer[4096];
    unsigned char arr[2];

    extern int optind;

    flag  = 0;

    while ((i = getopt(argc, argv, "hls?")) != EOF) switch (i) {
	case 'h':  flag |= FL_HEXOUT;	break;
	case 'l':  flag |= FL_LONGOUT;	break;
	case 's':  flag |= FL_SHORTOUT;	break;
  	default:   return usage(argv[0]); 
    }
    if (optind < argc) {
	return usage( argv[0]); 
    }

    if (!flag) flag = FL_LONGOUT;


    if ((fd = capi_open()) == -1) {
	perror("capi_open");
	return 1;
    }
    mode = fcntl( fd, F_GETFL);
    fcntl( fd, F_SETFL, mode &~ O_NDELAY);
    fcntl( fd, F_SETFL, mode &~ O_NONBLOCK);

    rn = capi_control_req( fd, 0, CTRL_CAPIREC_PLAY, 0);
    if (rn == CAPI_E_CONTROLLERFAILED) {
        fprintf( stderr, "capitrace switch failed, rn=0x%x", rn);
        return 1;
    }

    pollfd.fd      = fd;
    pollfd.events  = POLLIN;

    for(;;) {
	pollfd.revents = 0;

	switch (poll( &pollfd, 1, -1)) {
	    case -1:
		if (errno == EAGAIN || errno == EINTR) continue;
		fprintf( stderr, "poll failed\n");
		return 1;
	    default:
		continue; 
	    case 1:
		break;
	}
	/*
	 * read length of trace message
	 */
	rn = capi_blockread ( fd, arr, sizeof(arr));
	if (rn !=  sizeof(arr)) {
	    fprintf( stderr, "read length failed\n");
	    return 1;
	}
	len = (arr[0] << 8) | arr[1];
	len -= sizeof(arr);

	if (len > sizeof(buffer)) {
	    fprintf( stderr, "trace message too big = %d(0x%x)\n", len, len);
	    capi_hexdump( arr, sizeof(arr), 16, 2);
	    rn = capi_blockread( fd, buffer, sizeof(buffer));
	    capi_hexdump( buffer, sizeof(buffer), 16, 2);
	    return 1;
	}
	rn = capi_blockread( fd, buffer, len);
	if (rn != len) {
	    fprintf( stderr, "read trace message failed\n");
	    return 1;
	}
	/* skip over ack for trace control_req */
	if (cnt && len > sizeof(capitrace_t)+sizeof(CAPI_sheader_t)) {
	    dump_msg( buffer, len, flag);
	} else if (!cnt) {
	    fprintf( stderr, "CAPI Trace switched on\n");
	}
	cnt++;
    }
}
