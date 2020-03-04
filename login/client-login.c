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

static int login_init (void *pctx);
static void login_destroy (void *ctx);
static const char *login_response (void *ctx,
				   const char *challenge, int *len,
				   auth_interact_t interact, void *arg);

const struct auth_client_plugin sasl_client =
  {
  /* Plugin information */
    "LOGIN",
    "Non-standard LOGIN mechanism",
  /* Plugin instance */
    login_init,
    login_destroy,
  /* Authentication */
    login_response,
    AUTH_PLUGIN_PLAIN,
  /* Security Layer */
    0,
    NULL,
    NULL,
  };

static const struct auth_client_request client_request[] =
  {
    { "user",		AUTH_CLEARTEXT | AUTH_USER,	"User Name",	0, },
    { "passphrase",	AUTH_CLEARTEXT | AUTH_PASS,	"Password",	0, },
  };

struct login_context
  {
    int state;
    char *user;
    int user_len;
    char *pass;
    int pass_len;
  };
 
static int 
login_init (void *pctx)
{
  struct login_context *login;

  login = malloc (sizeof (struct login_context));
  memset (login, 0, sizeof (struct login_context));

  *(void **) pctx = login;
  return 1;
}

static void
login_destroy (void *ctx)
{
  struct login_context *login = ctx;

  if (login->user != NULL)
    {
      memset (login->user, '\0', login->user_len);
      free (login->user);
    }
  if (login->pass != NULL)
    {
      memset (login->pass, '\0', login->pass_len);
      free (login->pass);
    }
  free (login);
}

static const char *
login_response (void *ctx, const char *challenge __attribute__ ((unused)),
                int *len, auth_interact_t interact, void *arg)
{
  struct login_context *login = ctx;
  char *result[NELT (client_request)];

  switch (login->state)
    {
    case 0:
      /* Challenge is ignored (probably "Username:") */
      if (!(*interact) (client_request, result, NELT (client_request), arg))
        break;
      login->user = strdup (result[0]);
      login->user_len = strlen (login->user);
      login->pass = strdup (result[1]);
      login->pass_len = strlen (login->pass);
      login->state = 1;
      *len = login->user_len;
      return login->user;

    case 1:
      /* Challenge is ignored (probably "Password:") */
      login->state = -1;
      *len = login->pass_len;
      return login->pass;
    }
  *len = 0;
  return NULL;
}

