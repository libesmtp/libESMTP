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

/* Stuff to maintain a dynamically sized buffer for a string.  I seem
   to keep writing code like this, so I've put it in a file by itself.
   Maybe I should consolidate some other string buffering code with this.

   NOTE: no \0 terminating strings
 */

#include <config.h>

#include <assert.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <missing.h> /* declarations for missing library functions */

#include "concatenate.h"

/* malloc/realloc the buffer to be the requested length.
   Return 0 on failure, non-zero otherwise */
static int
cat_alloc (struct catbuf *catbuf, size_t length)
{
  char *nbuf;

  assert (catbuf != NULL && length > 0);

  if (catbuf->buffer == NULL)
    catbuf->buffer = malloc (length);
  else
    {
      nbuf = realloc (catbuf->buffer, length);
      if (nbuf == NULL)
	free (catbuf->buffer);
      catbuf->buffer = nbuf;
    }
  catbuf->allocated = (catbuf->buffer == NULL) ? 0 : length;
  if (catbuf->allocated < catbuf->string_length)
    catbuf->string_length = catbuf->allocated;
  return catbuf->buffer != NULL;
}

/* Reset the string to zero length without freeing the allocated memory */
void
cat_reset (struct catbuf *catbuf, size_t minimum_length)
{
  assert (catbuf != NULL);

  catbuf->string_length = 0;
  if (minimum_length > catbuf->allocated)
    cat_alloc (catbuf, minimum_length);
}

/* Initialise a buffer */
void
cat_init (struct catbuf *catbuf, size_t minimum_length)
{
  assert (catbuf != NULL);

  memset (catbuf, 0, sizeof (struct catbuf));
  if (minimum_length > 0)
    cat_alloc (catbuf, minimum_length);
}

/* Free memory allocated to the buffer */
void
cat_free (struct catbuf *catbuf)
{
  assert (catbuf != NULL);

  if (catbuf->buffer != NULL)
    free (catbuf->buffer);
  memset (catbuf, 0, sizeof (struct catbuf));
}

/* Return a pointer to the buffer and put its length in *len. */
char *
cat_buffer (struct catbuf *catbuf, int *len)
{
  assert (catbuf != NULL);

  if (len != NULL)
    *len = catbuf->string_length;
  return catbuf->buffer;
}

/* Shrink the allocated memory to the minimum needed.  Return a
   pointer to the buffer and put its length in *len. */
char *
cat_shrink (struct catbuf *catbuf, int *len)
{
  assert (catbuf != NULL);

  if (catbuf->buffer != NULL)
    cat_alloc (catbuf, catbuf->string_length);
  if (len != NULL)
    *len = catbuf->string_length;
  return catbuf->buffer;
}

/* Concatenate a string to the buffer.  N.B. the buffer is NOT terminated
   by a \0.  If len < 0 then string must be \0 terminated. */
char *
concatenate (struct catbuf *catbuf, const char *string, int len)
{
  size_t shortfall;

  assert (catbuf != NULL && string != NULL);

  if (len < 0)
    len = strlen (string);
  if (len > 0)
    {
      /* Ensure that the buffer is big enough to accept the string */
      if (catbuf->buffer == NULL)
	shortfall = 512;
      else
	shortfall = len - (catbuf->allocated - catbuf->string_length);
      if (shortfall > 0)
	{
	  if (shortfall % 128 != 0)
	    shortfall += 128 - shortfall % 128;
	  if (!cat_alloc (catbuf, catbuf->allocated + shortfall))
	    return NULL;
	}

      /* Copy the string */
      memcpy (catbuf->buffer + catbuf->string_length, string, len);
      catbuf->string_length += len;
    }
  return catbuf->buffer;
}

char *
vconcatenate (struct catbuf *catbuf, ...)
{
  va_list alist;
  const char *string;

  assert (catbuf != NULL);

  va_start (alist, catbuf);
  while ((string = va_arg (alist, const char *)) != NULL)
    concatenate (catbuf, string, -1);
  va_end (alist);
  return catbuf->buffer;
}

int
cat_printf (struct catbuf *catbuf, const char *format, ...)
{
  va_list alist;
  char buf[1024];
  int len;

  assert (catbuf != NULL && format != NULL);

  va_start (alist, format);
  len = vsnprintf (buf, sizeof buf, format, alist);
  va_end (alist);
  if (len <= 0)
    return len;
  if (len >= (int) sizeof buf)
    len = sizeof buf;
  concatenate (catbuf, buf, len);
  return len;
}
