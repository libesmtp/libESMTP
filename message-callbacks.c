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

/* Standard callback functions for use by message-source.c
   An application requiring anything more sophisticated than either of
   these will need to supply its own callback.  */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libesmtp.h"

/* Callback function to read the message from a file.  The file MUST be
   formatted according to RFC 2822 and lines MUST be terminated with the
   canonical CRLF sequence.  Furthermore, RFC 2821 line length
   limitations must be observed (1000 octets maximum). */
#define BUFLEN	8192

const char *
_smtp_message_fp_cb (void **ctx, int *len, void *arg)
{
  if (*ctx == NULL)
    *ctx = malloc (BUFLEN);

  if (len == NULL)
    {
      rewind ((FILE *) arg);
      return NULL;
    }

  *len = fread (*ctx, 1, BUFLEN, (FILE *) arg);
  return *ctx;
}

struct state
  {
    int state;
    int length;
  };

const char *
_smtp_message_str_cb (void **ctx, int *len, void *arg)
{
  const char *string = arg;
  struct state *state;

  if (*ctx == NULL)
    *ctx = malloc (sizeof (struct state));
  state = *ctx;

  if (len == NULL)
    {
      state->state = 0;
      state->length = strlen (string);
      return NULL;
    }

  switch (state->state)
    {
    case 0:
      state->state = 1;
      *len = state->length;
      break;

    default:
      *len = 0;
      break;
    }
  return string;
}
