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

#include <sys/types.h>
#include "hmacmd5.h"
#include "auth-client.h"
#include "auth-plugin.h"

#define NELT(x)		(sizeof x / sizeof x[0])

static int crammd5_init (void *pctx);
static void crammd5_destroy (void *ctx);
static const char *crammd5_response (void *ctx,
				     const char *challenge, int *len,
				     auth_interact_t interact, void *arg);

const struct auth_client_plugin sasl_client =
  {
  /* Plugin information */
    "CRAM-MD5",
    "Challenge-Response Authentication Mechanism (RFC 2195)",
  /* Plugin instance */
    crammd5_init,
    crammd5_destroy,
  /* Authentication */
    crammd5_response,
    0,
  /* Security Layer */
    0,
    NULL,
    NULL,
  };

static const struct auth_client_request client_request[] =
  {
    { "user",		AUTH_CLEARTEXT | AUTH_USER,	"User Name",	0, },
    { "passphrase",	AUTH_PASS,			"Pass Phrase",	0, },
  };

struct crammd5_context
  {
    int state;
    char *response;
    int response_len;
  };
 
static int 
crammd5_init (void *pctx)
{
  struct crammd5_context *context;

  context = malloc (sizeof (struct crammd5_context));
  memset (context, 0, sizeof (struct crammd5_context));

  *(void **) pctx = context;
  return 1;
}

static void
crammd5_destroy (void *ctx)
{
  struct crammd5_context *context = ctx;

  if (context->response != NULL)
    {
      memset (context->response, 0, context->response_len);
      free (context->response);
    }
  free (context);
}

static const char *
crammd5_response (void *ctx, const char *challenge, int *len,
		  auth_interact_t interact, void *arg)
{
  struct crammd5_context *context = ctx;
  char *result[NELT (client_request)];
  unsigned char digest[16];
  char *p, *response;
  int response_len;
  size_t i;
  static const char hex[] = "0123456789abcdef";

  switch (context->state)
    {
    case 0:	/* No initial response */
      context->state = 1;
      *len = 0;
      return NULL;

    case 1:	/* Digest the challenge and compute a response. */
      if (!(*interact) (client_request, result, NELT (client_request), arg))
        break;
      hmac_md5 (challenge, *len, result[1], strlen (result[1]), digest);
      response_len = strlen (result[0]) + 1 + 2 * sizeof digest;
      response = malloc (response_len);
      strcpy (response, result[0]);
      strcat (response, " ");
      p = strchr (response, '\0');
      for (i = 0; i < sizeof digest; i++)
        {
          *p++ = hex[(digest[i] >> 4) & 0x0F];
          *p++ = hex[digest[i] & 0x0F];
        }
      /* Note no \0 termination */
      context->state = -1;
      context->response = response;
      context->response_len = response_len;
      *len = response_len;
      return response;
    }
  *len = 0;
  return NULL;
}

