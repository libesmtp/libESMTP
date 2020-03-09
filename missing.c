/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2002  Brian Stafford  <brian@stafford.uklinux.net>
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
#include <missing.h>

#undef __EXTENSIONS__
#undef _GNU_SOURCE
#undef _ISOC9X_SOURCE
#undef _OSF_SOURCE
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE

#ifndef HAVE_STRDUP
#  include <string.h>
#  include <stdlib.h>
#endif
#if !defined(HAVE_STRCASECMP) || !defined(HAVE_STRNCASECMP)
#  include <ctype.h>
#endif

#ifndef HAVE_STRDUP
char *
strdup (const char *s1)
{
  char *p;

  p = malloc (strlen (s1) + 1);
  if (p != NULL)
    strcpy (p, s1);
  return p;
}
#endif

#ifndef HAVE_STRCASECMP
int
strcasecmp (const char *a, const char *b)
{
  register int n;

  while (*a == *b || (n = tolower (*a) - tolower (*b)) == 0)
    {
      if (*a == '\0')
        return 0;
      a++, b++;
    }
  return n;
}
#endif

#ifndef HAVE_STRNCASECMP
int
strncasecmp (const char *a, const char *b, size_t len)
{
  register int n;

  if (len < 1)
    return 0;
  while (*a == *b || (n = tolower (*a) - tolower (*b)) == 0)
    {
      if (*a == '\0' || --len < 1)
        return 0;
      a++, b++;
    }
  return n;
}
#endif

#ifndef HAVE_MEMRCHR
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-qual"

void *
memrchr (const void *a, int c, size_t len)
{
  const unsigned char *p = a;

  for (p += len - 1; (const void *) p >= a; p--)
    if (*p == c)
      return (void *) p;
  return (void *) 0;
}
# pragma GCC diagnostic pop
#endif

#ifndef HAVE_STRLCPY	/* adapted from glib */
size_t
strlcpy (char *dest, const char *src, size_t dest_size)
{
  char *d = dest;
  const char *s = src;
  size_t n = dest_size;

  if (n > 0)
    while (--n > 0)
      if ((*d++ = *s++) == '\0')
	break;

  if (n == 0)
    {
      if (dest_size > 0)
        *d = '\0';
      while (*s++)
        ;
    }
  return s - src - 1;
}
#endif
