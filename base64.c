/*
 *  This file is part of libESMTP, a library for submission of RFC 5322
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 4409 and RFC 5321.
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

#include <assert.h>

/* Routines to encode and decode base64 text.
 */
#include <ctype.h>
#include <string.h>
#include "base64.h"

/* RFC 2045 section 6.8 */

static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			     "abcdefghijklmnopqrstuvwxyz"
			     "0123456789"
			     "+/";

static const char index_64[128] =
  {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  };

/* Decode srclen bytes of base64 data contained in src and put the result
   in dst.  Since the destination buffer may contain arbitrary binary
   data and it is not necessarily a string, there is no \0 byte at the
   end of the decoded data. */
int
b64_decode (void *dst, int dstlen, const char *src, int srclen)
{
  const unsigned char *p, *q;
  unsigned char *t;
  int c1, c2;

  assert (dst != NULL && dstlen > 0);

  if (src == NULL)
    return 0;

  if (srclen < 0)
    srclen = strlen (src);

  /* Remove leading and trailing white space */
  for (p = (const unsigned char *) src;
       srclen > 0 && isspace (*p);
       p++, srclen--)
    ;
  for (q = p + srclen - 1; q >= p && isspace (*q); q--, srclen--)
    ;

  /* Length MUST be a multiple of 4 */
  if (srclen % 4 != 0)
    return -1;

  /* Destination buffer length must be sufficient */
  if (srclen / 4 * 3 + 1 > dstlen)
    return -1;

  t = dst;
  while (srclen > 0)
    {
      srclen -= 4;
      if (*p >= 128 || (c1 = index_64[*p++]) == -1)
        return -1;
      if (*p >= 128 || (c2 = index_64[*p++]) == -1)
        return -1;
      *t++ = (c1 << 2) | ((c2 & 0x30) >> 4);

      if (p[0] == '=' && p[1] == '=')
        break;
      if (*p >= 128 || (c1 = index_64[*p++]) == -1)
	return -1;
      *t++ = ((c2 & 0x0f) << 4) | ((c1 & 0x3c) >> 2);

      if (p[0] == '=')
	break;
      if (*p >= 128 || (c2 = index_64[*p++]) == -1)
        return -1;
      *t++ = ((c1 & 0x03) << 6) | c2;
    }

  return t - (unsigned char *) dst;
}

/* Return a pointer to a base 64 encoded string.  The input data is
   arbitrary binary data, the output is a \0 terminated string.
   src and dst may not share the same buffer.  */
int
b64_encode (char *dst, int dstlen, const void *src, int srclen)
{
  char *to = dst;
  const unsigned char *from;
  unsigned char c1, c2;
  int dst_needed;

  assert (dst != NULL && dstlen > 0 && srclen >= 0);

  if (src == NULL)
    return 0;

  dst_needed = (srclen + 2) / 3;
  dst_needed *= 4;
  if (dstlen < dst_needed + 1)
    return -1;

  from = src;
  while (srclen > 0)
    {
      c1 = *from++; srclen--;
      *to++ = base64[c1 >> 2];
      c1 = (c1 & 0x03) << 4;
      if (srclen <= 0)
	{
	  *to++ = base64[c1];
	  *to++ = '=';
	  *to++ = '=';
	  break;
	}
      c2 = *from++; srclen--;
      c1 |= (c2 >> 4) & 0x0f;
      *to++ = base64[c1];
      c1 = (c2 & 0x0f) << 2;
      if (srclen <= 0)
	{
	  *to++ = base64[c1];
	  *to++ = '=';
	  break;
	}
      c2 = *from++; srclen--;
      c1 |= (c2 >> 6) & 0x03;
      *to++ = base64[c1];
      *to++ = base64[c2 & 0x3f];
    }
  *to = '\0';
  return to - dst;
}

