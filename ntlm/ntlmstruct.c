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
#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ntlm.h"

/* Must have at least 32 bits in an int (at least pending a more thorough
   code review - this module is still experimental) */
#if SIZEOF_UNSIGNED_INT < 32 / 8
# error "unsigned int is less than 32 bits wide"
#endif

#if SIZEOF_UNSIGNED_SHORT == 16 / 8
typedef unsigned short unsigned16_t;
#else
#include <sys/types.h>
typedef	uint16 unsigned16_t;
#endif

#if SIZEOF_UNSIGNED_INT == 32 / 8
typedef	unsigned int unsigned32_t;
#elif SIZEOF_UNSIGNED_LONG == 32 / 8
typedef	unsigned long unsigned32_t;
#else
#include <sys/types.h>
typedef	uint32 unsigned32_t;
#endif

#ifdef WORDS_BIGENDIAN
/* Everything in NTLM is little endian binary, therefore byte swapping
   is needed on big endian platforms.  For simplicity, always provide
   byte swapping functions rather than trying to detect the local
   platform's support.	These functions make no effort to be efficient
   since this code doesn't require efficient byte swapping. */

#define SWAP(a,b)	do { unsigned char s = u.swap[(a)];	\
			     u.swap[(a)] = u.swap[(b)];		\
			     u.swap[(b)] = s; } while (0)

static unsigned16_t
bswap_16 (unsigned16_t value)
{
  union { unsigned16_t val; unsigned char swap[2]; } u;

  u.val = value;
  SWAP(0,1);
  return u.val;
}

static unsigned32_t
bswap_32 (unsigned32_t value)
{
  union u32 { unsigned32_t val; unsigned char swap[4]; } u;

  u.val = value;
  SWAP(0,3);
  SWAP(1,2);
  return u.val;
}

#undef SWAP
#endif

static void
write_uint16 (char *buf, size_t offset, unsigned int value)
{
  unsigned16_t i16 = value;

  assert (sizeof i16 == 2);
#ifdef WORDS_BIGENDIAN
  i16 = bswap_16 (i16);
#endif
  memcpy (buf + offset, &i16, sizeof i16);
}

static inline void
write_uint32 (char *buf, size_t offset, unsigned int value)
{
  unsigned32_t i32 = value;

  assert (sizeof i32 == 4);
#ifdef WORDS_BIGENDIAN
  i32 = bswap_32 (i32);
#endif
  memcpy (buf + offset, &i32, sizeof i32);
}

static inline unsigned int
read_uint32 (const char *buf, size_t offset)
{
  unsigned32_t i32;

  assert (sizeof i32 == 4);
  memcpy (&i32, buf + offset, sizeof i32);
#ifdef WORDS_BIGENDIAN
  i32 = bswap_32 (i32);
#endif
  return i32;
}

static inline void
write_string (char *buf, size_t offset, size_t *str_offset,
	      const void *data, size_t len)
{
  if (data == NULL)
    len = 0;
  write_uint16 (buf, offset, len);
  write_uint16 (buf, offset + 2, len);
  write_uint32 (buf, offset + 4, *str_offset);
  if (len > 0)
    memcpy (buf + *str_offset, data, len);
  *str_offset += len;
}

/* Offsets into on-the-wire NTLM structures */
#define PROTOCOL			0	/* "NTLMSSP" */
#define MSGTYPE				8	/* 1..3 */
/* Type 1 fields */
#define T1FLAGS				12	/* 0xb203 */
#define T1DOMAIN			16	/* domain to authenticate in */
#define T1WKSTN				24	/* client workstation name */
#define T1SIZE				32
/* Type 2 fields */
#define T2AUTHTARGET			12	/* domain/server */
#define T2FLAGS				20	/* 0x8201 */
#define T2NONCE				24	/* server challenge */
#define T2RESERVED			32
#define T2SIZE				40
/* Type 3 fields */
#define T3LMRESPONSE			12	/* lanmanager hash */
#define T3NTRESPONSE			20	/* nt hash */
#define T3DOMAIN			28	/* domain to authenticate against */
#define T3USER				36	/* user name */
#define T3WKSTN				44	/* client workstation name */
#define T3SESSIONKEY			52	/* client workstation name */
#define T3FLAGS				60	/* 0xb203 */
#define T3SIZE				64

static const char NTLMSSP[] = "NTLMSSP";

/* Build a NTLM type 1 structure in the buffer.
	domain - the NT domain the workstation belongs to
   workstation - the NT (netbios) name of the workstation */
size_t
ntlm_build_type_1 (char *buf, size_t buflen, unsigned int flags,
		   const char *domain, const char *workstation)
{
  size_t offset = T1SIZE;
  size_t len;
  char string[256];

  if (buflen < offset)
    return 0;
  memcpy (buf, NTLMSSP, 8);
  write_uint32 (buf, MSGTYPE, 1);
  write_uint32 (buf, T1FLAGS, flags);
  len = lm_uccpy (string, sizeof string, domain);
  if (offset + len > buflen)
    return 0;
  write_string (buf, T1DOMAIN, &offset, string, len);
  len = lm_uccpy (string, sizeof string, workstation);
  if (offset + len > buflen)
    return 0;
  write_string (buf, T1WKSTN, &offset, string, len);
  return offset;
}

/* Build a NTLM type 2 structure in the buffer */
size_t
ntlm_build_type_2 (char *buf, size_t buflen, unsigned int flags,
		   const unsigned char *nonce, const char *domain)
{
  size_t offset = T2SIZE;
  size_t len;
  char string[256];
  unsigned char *up;

  if (buflen < offset)
    return 0;
  memcpy (buf, NTLMSSP, 8);
  write_uint32 (buf, MSGTYPE, 2);
  len = lm_uccpy (string, sizeof string, domain);
  if (offset + 2 * len > buflen)
    return 0;
  up = nt_unicode (string, len);
  write_string (buf, T2AUTHTARGET, &offset, up, 2 * len);
  if (up != NULL)
    free (up);
  write_uint32 (buf, T2FLAGS, flags);
  memcpy (buf + T2NONCE, nonce, 8);
  memset (buf + T2RESERVED, 0, 8);
  return offset;
}

/* Build a NTLM type 3 structure in the buffer */
size_t
ntlm_build_type_3 (char *buf, size_t buflen, unsigned int flags,
		   const unsigned char *lm_resp, const unsigned char *nt_resp,
		   const char *domain, const char *user, const char *workstation)
{
  size_t offset = T3SIZE;
  size_t len;
  char string[256];
  unsigned char *up;

  if (buflen + 24 + 24 < offset)
    return 0;
  memcpy (buf, NTLMSSP, 8);
  write_uint32 (buf, MSGTYPE, 3);
  write_string (buf, T3LMRESPONSE, &offset, lm_resp, 24);
  write_string (buf, T3NTRESPONSE, &offset, nt_resp, 24);
  len = lm_uccpy (string, sizeof string, domain);
  if (offset + 2 * len > buflen)
    return 0;
  up = nt_unicode (string, len);
  write_string (buf, T3DOMAIN, &offset, up, 2 * len);
  if (up != NULL)
    free (up);
  len = lm_uccpy (string, sizeof string, user);
  if (offset + 2 * len > buflen)
    return 0;
  up = nt_unicode (string, len);
  write_string (buf, T3USER, &offset, up, 2 * len);
  if (up != NULL)
    free (up);
  len = lm_uccpy (string, sizeof string, workstation);
  if (offset + 2 * len > buflen)
    return 0;
  up = nt_unicode (string, len);
  write_string (buf, T3WKSTN, &offset, up, 2 * len);
  if (up != NULL)
    free (up);
  write_string (buf, T3SESSIONKEY, &offset, NULL, 0);
  write_uint32 (buf, T3FLAGS, flags);
  return offset;
}

/****/

/* Parse a NTLM type 2 structure in the buffer (partial implementation).
   Verify that the packet is a type 2 structure and copy the nonce to the
   supplied buffer (which must be eight bytes long) */
size_t
ntlm_parse_type_2 (const char *buf, size_t buflen, unsigned int *flags,
		   unsigned char *nonce, char **domain)
{
  if (buflen < T2SIZE)
    return 0;
  if (memcmp (buf, NTLMSSP, 8) != 0)
    return 0;
  if (read_uint32 (buf, MSGTYPE) != 2)
    return 0;
  *flags = read_uint32 (buf, T2FLAGS);
  *domain = NULL;
  memcpy (nonce, buf + T2NONCE, 8);
  return 1;
}

