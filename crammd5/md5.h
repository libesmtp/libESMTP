/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

/* parts of this file are :
 * Written March 1993 by Branko Lankester
 * Modified June 1993 by Colin Plumb for altered md5.c.
 * Modified October 1995 by Erik Troan for RPM
 */
#ifndef MD5_UTILS_H
#define MD5_UTILS_H

#if SIZEOF_UNSIGNED_INT == 32 / 8
typedef	unsigned int unsigned32_t;
#elif SIZEOF_UNSIGNED_LONG == 32 / 8
typedef	unsigned long unsigned32_t;
#else
#include <sys/types.h>
typedef	uint32 unsigned32_t;
#endif

#include <sys/types.h>

typedef struct {
	unsigned32_t buf[4];
	unsigned32_t bits[2];
	unsigned char in[64];
	int doByteReverse;
} MD5Context ;

/* raw routines */
void md5_init (MD5Context *ctx);
void md5_update (MD5Context *ctx, const void *buf, size_t len);
void md5_final (MD5Context *ctx, unsigned char digest[16]);


#endif	/* MD5_UTILS_H */
