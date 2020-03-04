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

/*  Functions to read lines or blocks of text from the message source.
    These functions allow the library to interface to the application
    using a callback function.  This is intended to allow the application
    maximum flexibility in managing its message storage.  */

#include <config.h>

#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include "message-source.h"

/* This is similar to code in siobuf.c */

struct msg_source
  {
    /* Callback to fill the input buffer */
    const char *(*cb) (void **ctx, int *len, void *arg);
    void *arg;
    void *ctx;

    /* Input buffer */
    const char *rp;		/* input buffer pointer */
    int rn;			/* number of bytes unread in buffer */

    /* Output buffer (used by msg_gets()) */
    char *buf;
    size_t nalloc;
  };

msg_source_t
msg_source_create (void)
{
  msg_source_t source;

  if ((source = malloc (sizeof (struct msg_source))) != NULL)
    memset (source, 0, sizeof (struct msg_source));
  return source;
}

void
msg_source_destroy (msg_source_t source)
{
  assert (source != NULL);

  if (source->ctx != NULL)
    free (source->ctx);
  if (source->buf != NULL)
    free (source->buf);
  free (source);
}

void
msg_source_set_cb (msg_source_t source,
                   const char *(*cb) (void **ctx, int *len, void *arg),
                   void *arg)
{
  assert (source != NULL);

  if (source->ctx != NULL)
    {
      free (source->ctx);
      source->ctx = NULL;
    }
  source->cb = cb;
  source->arg = arg;
}

/* Use the callback to get data from the message source.
 */
static int
msg_fill (msg_source_t source)
{
  assert (source != NULL && source->cb != NULL);

  source->rp = (*source->cb) (&source->ctx, &source->rn, source->arg);
  return source->rn > 0;
}

void
msg_rewind (msg_source_t source)
{
  assert (source != NULL && source->cb != NULL);

  (*source->cb) (&source->ctx, NULL, source->arg);
}

/* Line oriented reader.  An output buffer is allocated as required.
   The return value is a pointer to the line and remains valid until the
   next call to msg_gets ().  The line is guaranteed to be terminated
   with a \r\n.  Len is set to the number of octets in the buffer.
   The line is *not* terminated with a \0.

   If concatenate is non-zero, the next line of input is concatenated
   with the existing line and the return value points to the original
   line in the buffer.  In this case *len is the number of octets in
   the buffer when called and is updated to the new count on return.
 */
const char *
msg_gets (msg_source_t source, int *len, int concatenate)
{
  int lastc, c, buflen;
  char *p, *nbuf;

  assert (source != NULL && len != NULL);

  if (source->rn <= 0 && !msg_fill (source))
    return NULL;

  if (source->buf == NULL)
    {
      source->nalloc = 1023;	/* RFC 5321 max line length + slack */
      source->buf = malloc (source->nalloc + 2);
      if (source->buf == NULL)
        return NULL;
    }
  p = source->buf;
  buflen = source->nalloc;
  if (concatenate)
    {
      p += *len;
      buflen -= *len;
    }

  lastc = 0;
  while (source->rn > 0 || msg_fill (source))
    {
      c = *source->rp++;
      source->rn--;
      if (buflen <= 0)
	{
	  buflen = 512;
	  source->nalloc += buflen;
	  nbuf = realloc (source->buf, source->nalloc + 2);
	  if (nbuf == NULL)
	    {
	      free (source->buf);
	      return NULL;
	    }
	  p = nbuf + (p - source->buf);
	  source->buf = nbuf;
	}
      *p++ = c;
      buflen--;
      if (c == '\n' && lastc == '\r')
	{
	  *len = p - source->buf;
	  return source->buf;
	}
      lastc = c;
    }
  /* Only get here if the input was not properly terminated with a \r\n.
     The handling of the DATA command in protocol.c relies on the \n
     so we add it here.  This is why there is 2 characters of slack in
     malloc and realloc above. */
  if (lastc != '\r')
    *p++ = '\r';
  *p++ = '\n';
  *len = p - source->buf;
  return source->buf;
}

/* Return the next character in the source, i.e. the first character
   that will be returned by the next call to msg_gets().  This is
   currently only used to check for RFC 5322 header continuation lines.
   It is not safe to use in conjunction with msg_getb().
 */
int
msg_nextc (msg_source_t source)
{
  assert (source != NULL);

  if (source->rn <= 0 && !msg_fill (source))
    return -1;

  return *source->rp;
}

/* Block oriented reader.  The output buffer is not used for efficiency.
 */
const char *
msg_getb (msg_source_t source, int *len)
{
  assert (source != NULL);

  if (source->rn <= 0 && !msg_fill (source))
    return NULL;

  /* Just return whatever is in the input buffer */
  *len = source->rn;
  source->rn = 0;
  return source->rp;
}
