/*
 ***********************************************************************
 ** md5.h -- header file for implementation of MD5                    **
 ** RSA Data Security, Inc. MD5 Message-Digest Algorithm              **
 ** Created: 2/17/90 RLR                                              **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version               **
 ** Revised (for MD5): RLR 4/27/91                                    **
 **   -- G modified to have y&~z instead of y&z                       **
 **   -- FF, GG, HH modified to add in last register done             **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3     **
 **   -- distinct additive constant for each step                     **
 **   -- round 4 added, working mod 7                                 **
 ***********************************************************************
 */

#ifndef MD5_H
#define MD5_H

#include "capiconf.h"

#ifdef _WINDOWS
#define PROTOTYPE	1
#endif

#ifdef __STDC__
#define PROTOTYPE	1
#endif

#ifdef __cplusplus
#define PROTOTYPE	1
extern "C" {
#endif

/* Data structure for MD5 (Message-Digest) computation */
#define MD5_SZ 16

typedef struct md5context {
  UINT4 i[2];            /* number of _bits_ handled mod 2^64 */
  UINT4 buf[4];                             /* scratch buffer */
  unsigned char in[64];                              /* input buffer */
  unsigned char digest[MD5_SZ]; /* actual digest after MD5Final call */
} md5context_t;

#ifdef PROTOTYPE
void MD5Init(md5context_t *);
void MD5Update(md5context_t *, unsigned char *, UINT4);
void MD5Final(md5context_t *);
#else
void MD5Init();
void MD5Update();
void MD5Final();
#endif

#ifdef __cplusplus
}
#endif

#endif	/* MD5_H */
