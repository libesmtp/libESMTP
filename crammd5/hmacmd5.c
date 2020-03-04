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
#include <config.h>

#include <string.h>
#include <sys/types.h>
#include "hmacmd5.h"

#define PAD_SIZE	64

/*
 * the HMAC_MD5 transform looks like:
 *
 * MD5(K XOR opad, MD5(K XOR ipad, challenge))
 *
 * where K is an n byte secret
 * ipad is the byte 0x36 repeated 64 times
 * opad is the byte 0x5c repeated 64 times
 * and challenge is the data being protected
 */

/* Precompute HMAC-MD5 contexts from a secret
 */
void
hmac_md5_pre (const void *secret, size_t secret_len,
              MD5_CTX *inner, MD5_CTX *outer)
{
  unsigned char ipad[PAD_SIZE];
  unsigned char opad[PAD_SIZE];
  unsigned char tk[16];
  int i;

  /* If secret is longer than 64 bytes reset it to secret = MD5 (secret)
   */
  if (secret_len > PAD_SIZE)
    {
      MD5_CTX tctx;

      MD5_Init (&tctx);
      MD5_Update (&tctx, secret, secret_len);
      MD5_Final (tk, &tctx);
      secret = tk;
      secret_len = sizeof tk;
    }

  /* start out by storing secret in pads */
  memcpy (ipad, secret, secret_len);
  if (secret_len < sizeof ipad)
    memset (ipad + secret_len, 0, sizeof ipad - secret_len);
  memcpy (opad, secret, secret_len);
  if (secret_len < sizeof opad)
    memset (opad + secret_len, 0, sizeof opad - secret_len);

  /* XOR secret with ipad and opad values */
  for (i = 0; i < PAD_SIZE; i++)
    {
      ipad[i] ^= 0x36;
      opad[i] ^= 0x5c;
    }

  /* perform inner MD5 */
  MD5_Init (inner);
  MD5_Update (inner, ipad, sizeof ipad);

  /* perform outer MD5 */
  MD5_Init (outer);
  MD5_Update (outer, opad, sizeof opad);
}

/* Finalise HMAC-MD5 contexts from a challenge
 */
void
hmac_md5_post (const void *challenge, size_t challenge_len,
               MD5_CTX *inner, MD5_CTX *outer, unsigned char digest[16])
{
  unsigned char id[16];

  /* perform inner MD5 */
  MD5_Update (inner, challenge, challenge_len);
  MD5_Final (id, inner);

  /* perform outer MD5 */
  MD5_Update (outer, id, sizeof id);
  MD5_Final (digest, outer);
}

/* Digest a challenge and a secret.
 */
void
hmac_md5 (const void *challenge, size_t challenge_len,
	  const void *secret, size_t secret_len,
	  unsigned char digest[16])
{
  MD5_CTX inner, outer;

  hmac_md5_pre (secret, secret_len, &inner, &outer);
  hmac_md5_post (challenge, challenge_len, &inner, &outer, digest);
}

