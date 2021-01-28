/*
 *  Authentication module for the Micr$oft NTLM mechanism.
 *
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2002	Brian Stafford	<brian@stafford.uklinux.net>
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <openssl/des.h>
#include <openssl/md4.h>

#include "ntlm.h"

static void
lm_deshash (void *result, const_DES_cblock *iv, const void *secret)
{
  DES_cblock key;
  DES_key_schedule ks;
  unsigned char key_56[8];
  size_t len;

  /* copy and pad the secret */
  len = strlen (secret);
  if (len > sizeof key_56)
    len = sizeof key_56;
  memcpy (key_56, secret, len);
  if (sizeof key_56 - len > 0)
    memset (key_56 + len, 0, sizeof key_56 - len);

  /* convert 56 bit key to the 64 bit */
  key[0] = key_56[0];
  key[1] = (key_56[0] << 7) | (key_56[1] >> 1);
  key[2] = (key_56[1] << 6) | (key_56[2] >> 2);
  key[3] = (key_56[2] << 5) | (key_56[3] >> 3);
  key[4] = (key_56[3] << 4) | (key_56[4] >> 4);
  key[5] = (key_56[4] << 3) | (key_56[5] >> 5);
  key[6] = (key_56[5] << 2) | (key_56[6] >> 6);
  key[7] = (key_56[6] << 1);

  DES_set_odd_parity (&key);
  DES_set_key (&key, &ks);
  DES_ecb_encrypt (iv, result, &ks, DES_ENCRYPT);

  /* paranoia */
  memset (key, 0, sizeof key);
  memset (&ks, 0, sizeof ks);
}

/* Copy and convert to upper case.  If supplied string is shorter than the
   destination, zero pad the remainder. */
int
lm_uccpy (char *dst, size_t dstlen, const char *src)
{
  char *p;
  size_t len;

  if ((len = src != NULL ? strlen (src) : 0) > dstlen)
    len = dstlen;
  for (p = dst; len > 0; p++, src++, len--)
    *p = toupper (*src);
  if (p < dst + dstlen)
    memset (p, 0, dst + dstlen - p);
  return len;
}

/* create LanManager hashed password */
void
lm_hash_password (unsigned char *hash, const char *pass)
{
  static const_DES_cblock iv = { 0x4B, 0x47, 0x53, 0x21,
				 0x40, 0x23, 0x24, 0x25 };
  char lmpass[14];

  lm_uccpy (lmpass, sizeof lmpass, pass);
  lm_deshash (hash, &iv, lmpass);
  lm_deshash (hash + 8, &iv, lmpass + 7);
  memset (lmpass, 0, sizeof lmpass);
}

/* convert to unicode */
unsigned char *
nt_unicode (const char *string, size_t len)
{
  unsigned char *uni, *pp;

  if (len == 0)
    return NULL;

  uni = malloc (len * 2);
  if ((pp = uni) != NULL)
    while (len-- > 0)
      {
	*pp++ = (unsigned char) *string++;
	*pp++ = 0;
      }
  return uni;
}

void
nt_hash_password (unsigned char *hash, const char *pass)
{
  int len;
  unsigned char *nt_pw;
  MD4_CTX context;

  len = strlen (pass);
  if ((nt_pw = nt_unicode (pass, len)) == NULL)
    return;

  MD4_Init (&context);
  MD4_Update (&context, nt_pw, 2 * len);
  MD4_Final (hash, &context);
  memset (&context, 0, sizeof context);
  memset (nt_pw, 0, 2 * len);
  free (nt_pw);
}

/* Use the server's 8 octet nonce and the secret to create the 24 octet
   LanManager and NT responses. */
void
ntlm_responses (unsigned char *lm_resp, unsigned char *nt_resp,
		const unsigned char *challenge, const char *secret)
{
  unsigned char hash[21];
  DES_cblock nonce;

  memcpy (&nonce, challenge, sizeof nonce);

  lm_hash_password (hash, secret);
  memset (hash + 16, 0, 5);
  lm_deshash (lm_resp, &nonce, hash);
  lm_deshash (lm_resp + 8, &nonce, hash + 7);
  lm_deshash (lm_resp + 16, &nonce, hash + 14);

  nt_hash_password (hash, secret);
  memset (hash + 16, 0, 5);
  lm_deshash (nt_resp, &nonce, hash);
  lm_deshash (nt_resp + 8, &nonce, hash + 7);
  lm_deshash (nt_resp + 16, &nonce, hash + 14);
  memset (hash, 0, sizeof hash);
}

