#ifndef _hmac_md5_h
#define _hmac_md5_h
/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2001,2002  Brian Stafford  <brian@stafford.uklinux.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_LIBCRYPTO

#include <openssl/md5.h>

#else

#include "md5.h"

#define MD5_CTX			MD5Context
#define MD5_Init(c)		md5_init((c))
#define MD5_Update(c,d,l)	md5_update((c),(d),(l))
#define MD5_Final(md,c)		md5_final((c),(md))

#endif

/* Precompute HMAC-MD5 contexts from a secret. */
void hmac_md5_pre (const void *secret, size_t secret_len,
                   MD5_CTX *inner, MD5_CTX *outer);
/* Finalise HMAC-MD5 contexts from a challenge.  */
void hmac_md5_post (const void *challenge, size_t challenge_len,
                    MD5_CTX *inner, MD5_CTX *outer, unsigned char digest[16]);
/* Digest a challenge and a secret.  */
void hmac_md5 (const void *challenge, size_t challenge_len,
	       const void *secret, size_t secret_len,
	       unsigned char digest[16]);

#endif
