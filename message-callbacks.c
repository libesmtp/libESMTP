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
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libesmtp.h"

#define BUFLEN	8192

/**
 * DOC: Message Callbacks.
 *
 * Message Callbacks
 * -----------------
 *
 * libESMTP provides basic message callbacks to handle two common cases,
 * reading from a file and reading from a string.  In both cases the message
 * *must* be formatted according to RFC 5322 and lines *must* be terminated
 * with the canonical CRLF sequence.  Furthermore, RFC 5321 line length
 * limitations must be observed (1000 octets maximum).
 */

/**
 * _smtp_message_fp_cb() - Read message from a file.
 * @ctx: context data for the message.
 * @len: length of message data returned to the application.
 * @arg: application data (closure) passed to the callback.
 *
 * Callback function to read the message from a file.
 *
 * Return: A pointer to message data which remains valid until the next call.
 */
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

/**
 * _smtp_message_str_cb() - Read message from a string.
 * @ctx: context data for the message.
 * @len: length of message data returned to the application.
 * @arg: application data (closure) passed to the callback.
 *
 * Callback function to read the message from a string.
 *
 * Return: A pointer to message data which remains valid until the next call.
 */
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
