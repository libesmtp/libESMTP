/*
 *  Authentication module for the Micr$oft NTLM mechanism.
 *
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
#define _XOPEN_SOURCE	500

#include <config.h>

#include <assert.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include "ntlm.h"
#include "auth-client.h"
#include "auth-plugin.h"

#if 0
/* This is handy for changing te value of the flags in the debugger. */
unsigned int t1flags = TYPE1_FLAGS;
#undef TYPE1_FLAGS
#define TYPE1_FLAGS t1flags
#endif

#define NELT(x)		(sizeof x / sizeof x[0])

static int ntlm_init (void *pctx);
static void ntlm_destroy (void *ctx);
static const char *ntlm_response (void *ctx,
				  const char *challenge, int *len,
				  auth_interact_t interact, void *arg);

const struct auth_client_plugin sasl_client =
  {
  /* Plugin information */
    "NTLM",
    "NTLM Authentication Mechanism (Microsoft)",
  /* Plugin instance */
    ntlm_init,
    ntlm_destroy,
  /* Authentication */
    ntlm_response,
    0,
  /* Security Layer */
    0,
    NULL,
    NULL,
  };

static const struct auth_client_request client_request[] =
  {
    { "domain",		AUTH_CLEARTEXT | AUTH_REALM,	"Domain",	0, },
    { "user",		AUTH_CLEARTEXT | AUTH_USER,	"User Name",	0, },
    { "passphrase",	AUTH_PASS,			"Pass Phrase",	0, },
  };

struct ntlm_context
  {
    int state;
    char *result[NELT (client_request)];
    char host[64];
    char buf[256];
  };
 
static int 
ntlm_init (void *pctx)
{
  struct ntlm_context *context;

  context = malloc (sizeof (struct ntlm_context));
  memset (context, 0, sizeof (struct ntlm_context));

  *(void **) pctx = context;
  return 1;
}

static void
ntlm_destroy (void *ctx)
{
  struct ntlm_context *context = ctx;

  memset (context, 0, sizeof (struct ntlm_context));
  free (context);
}

static const char *
ntlm_response (void *ctx, const char *challenge, int *len,
	       auth_interact_t interact, void *arg)
{
  struct ntlm_context *context = ctx;
  unsigned char nonce[8];
  unsigned char lm_resp[24], nt_resp[24];
  unsigned int flags;
  char *domain = NULL;
  char *p;

  switch (context->state)
    {
    case 0:	/* build the authentication request */
      context->state = 1;
      if (!(*interact) (client_request, context->result, NELT (client_request),
                        arg))
        break;
      gethostname (context->host, sizeof context->host);
      if ((p = strchr (context->host, '.')) != NULL)
        *p = '\0';
      *len = ntlm_build_type_1 (context->buf, sizeof context->buf, TYPE1_FLAGS,
      				context->result[0], context->host);
      return context->buf;

    case 1:	/* compute a response based om the challenge */
      context->state = 2;
      if (!ntlm_parse_type_2 (challenge, *len, &flags, nonce, &domain))
        break;
      ntlm_responses (lm_resp, nt_resp, nonce, context->result[2]);
      *len = ntlm_build_type_3 (context->buf, sizeof context->buf,
				flags, lm_resp, nt_resp,
				context->result[0], context->result[1],
				context->host);
      if (domain != NULL)
        free (domain);
      return context->buf;
    }
  *len = 0;
  return NULL;
}

