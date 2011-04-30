/******************************************************************************
 *  (C)opyright 1991-1999 BinTec Communications AG, All Rights Reserved
 *
 *       Title: <one line description>
 *      Author: <username>
 *    $RCSfile: poll.c,v $
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
    static const char _rcsid_poll_c[] __UNUSED = "$Id: poll.c,v 1.3 2002/08/25 00:42:18 shoki Exp $";
#endif

#include "capiconf.h"

int poll( pfds, nfds, timeout)
struct pollfd *pfds;
int nfds;
int timeout;
{
    int maxfd=0, cnt, rv;
    struct pollfd *pf, *end = pfds+nfds;
    fd_set readfds, writefds, exceptfds;
    struct timeval timeval, *tvp;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    for(pf = pfds; pf < end; pf++) {
	if (pf->events & POLLIN)  FD_SET( pf->fd, &readfds);
	if (pf->events & POLLOUT) FD_SET( pf->fd, &writefds);
	if (pf->events & POLLPRI) FD_SET( pf->fd, &exceptfds);
	pf->revents = 0;
	maxfd = pf->fd > maxfd ? pf->fd : maxfd;
    }
    maxfd++;

    tvp = &timeval;
    if (timeout == 0) {			/*  return immediately  */
	tvp->tv_sec  = 0;
	tvp->tv_usec = 0;
    } else if (timeout == -1) {		/*  wait forever  */
	tvp = NULL;
    } else {				/*  return after timeout  */
	tvp->tv_sec  = timeout / 1000;
	tvp->tv_usec = (timeout % 1000) * 1000;
    }

    if ((rv = select(maxfd, &readfds, &writefds, &exceptfds, tvp)) != -1) {
	rv = 0;
	for(pf = pfds; pf < end; pf++) {
	    pf->revents = 0;
	    cnt = 0;
	    if (FD_ISSET(pf->fd, &readfds)) {
		pf->revents |= POLLIN;
		cnt++;
	    }
	    if (FD_ISSET(pf->fd, &writefds)) {
		pf->revents |= POLLOUT;
		cnt++;
	    }
	    if (FD_ISSET(pf->fd, &exceptfds)) {
		pf->revents |= POLLPRI;
		cnt++;
	    }
	    if (cnt) rv++;
	}
    }
    return (rv);
}


