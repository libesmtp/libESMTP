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

#include <assert.h>

#include <string.h>
#include <stdlib.h>

#include "auth-client.h"
#include "auth-plugin.h"

#define NELT(x)		(sizeof x / sizeof x[0])

static int plain_init (void *pctx);
static void plain_destroy (void *ctx);
static const char *plain_response (void *ctx, const char *challenge, int *len,
				   auth_interact_t interact, void *arg);

const struct auth_client_plugin sasl_client =
  {
  /* Plugin information */
    "PLAIN",
    "PLAIN mechanism (RFC 2595 section 6)",
  /* Plugin instance */
    plain_init,
    plain_destroy,
  /* Authentication */
    plain_response,
    AUTH_PLUGIN_PLAIN,
  /* Security Layer */
    0,
    NULL,
    NULL,
  };

static const struct auth_client_request client_request[] =
  {
    { "user",		AUTH_CLEARTEXT | AUTH_USER,	"User Name",	255, },
    { "passphrase",	AUTH_CLEARTEXT | AUTH_PASS,	"Pass Phrase",	255, },
  };

struct plain_context
  {
    int state;
    char buf[2 * 255 + 3];
  };
 
static int 
plain_init (void *pctx)
{
  struct plain_context *plain;

  plain = malloc (sizeof (struct plain_context));
  memset (plain, 0, sizeof (struct plain_context));

  *(void **) pctx = plain;
  return 1;
}

static void
plain_destroy (void *ctx)
{
  struct plain_context *plain = ctx;

  memset (plain->buf, '\0', sizeof plain->buf);
  free (plain);
}

static const char *
plain_response (void *ctx, const char *challenge __attribute__ ((unused)),
                int *len, auth_interact_t interact, void *arg)
{
  struct plain_context *plain = ctx;
  char *result[NELT (client_request)];

  switch (plain->state)
    {
    case 0:
      if (!(*interact) (client_request, result, NELT (client_request), arg))
        break;
      strcpy (plain->buf + 1, result[0]);
      strcpy (plain->buf + strlen (result[0]) + 2, result[1]);
      *len = strlen (result[0]) + strlen (result[1]) + 2;
      plain->state = -1;
      return plain->buf;
    }
  *len = 0;
  return NULL;
}

