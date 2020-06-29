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

#include <stdlib.h>
#include <string.h>

#include "tokens.h"

#define ATOMEXCLUDE	"\"()<>[]@,;:\\."
#define XTEXTEXCLUDE	" +="
#define WHITESPACE	" \t\r\n\v"

#define SPACE		1
#define GRAPH		2
#define ATOM		4
#define XTEXT		8

static char atomchars[256];
#define is_atom(c)	(atomchars[(int) (c)] & ATOM)
#define is_xtext(c)	(atomchars[(int) (c)] & XTEXT)
#define is_space(c)	(atomchars[(int) (c)] & SPACE)

#define initatom()	do { if (!is_space (' ')) _initatom (); } while (0) 

static void
_initatom (void)
{
  const char *p;
  int i;

  /* Mark characters in the printable ASCII range */
  for (i = ' ' + 1; i <= '~'; i++)
    atomchars[i] |= (ATOM | GRAPH | XTEXT);

  /* Turn off the ATOM flag on characters that are excluded */
  for (p = ATOMEXCLUDE; *p != '\0'; p++)
    atomchars[(int) *p] &= ~ATOM;

  /* Turn off the XTEXT flag on printable characters that should be encoded */
  for (p = XTEXTEXCLUDE; *p != '\0'; p++)
    atomchars[(int) *p] &= ~XTEXT;

  /* Mark space characters */
  for (p = WHITESPACE; *p != '\0'; p++)
    atomchars[(int) *p] |= SPACE;
}

const char *
skipblank (const char *s)
{
  initatom ();
  while (is_space (*s))
    s++;
  return s;
}

int
read_atom (const char *s, const char **es, char buf[], int len)
{
  char *t;

  initatom ();
  if (!is_atom (*s))
    return 0;
  t = buf;
  do
    {
      if (t < buf + len - 1)
	*t++ = *s;
      s++;
    }
  while (is_atom (*s));
  *t = '\0';
  if (es != NULL)
    *es = s;
  return t - buf;
}

static char xdigits[] = "0123456789ABCDEF";

/* Return a pointer to an xtext encoded string.
 */
char *
encode_xtext (char buf[], int len, const char *string)
{
  const unsigned char *s;
  char *t;

  for (s = (const unsigned char *) string, t = buf; *s != '\0'; s++, t++)
    {
      if (t - buf > len - 1)
        return NULL;
      if (is_xtext (*s))
	*t = *s;
      else
	{
	  *t++ = '+';
	  *t++ = xdigits[(*s & 0xF0) >> 4];
	  *t = xdigits[*s & 0x0F];
	}
    }
  *t = '\0';
  return buf;
}

