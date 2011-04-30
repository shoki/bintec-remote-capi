#ifndef __POLL_H
#define __POLL_H

struct pollfd {
    int fd;
    short events;
    short revents;
};

#define POLLIN		01
#define POLLPRI		02
#define POLLOUT		04

#ifdef __STDC__
extern int poll( struct pollfd *, int, int );
#else
extern int poll();
#endif

#endif	/*  __POLL_H  */


