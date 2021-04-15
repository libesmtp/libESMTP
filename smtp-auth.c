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

/* Support for the SMTP AUTH verb.
 */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include <missing.h> /* declarations for missing library functions */

#include "auth-client.h"
#include "libesmtp-private.h"
#include "api.h"

#include "message-source.h"
#include "siobuf.h"
#include "tokens.h"
#include "base64.h"
#include "protocol.h"

/**
 * DOC: RFC 4954
 *
 * Auth Extension
 * --------------
 *
 * When enabled and the SMTP server advertises the AUTH extension, libESMTP
 * will attempt to authenticate to the SMTP server before transferring any
 * messages.
 *
 * Authentication Contexts
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *
 * A separate authentication context must be created for each SMTP session.
 * The application is responsible for destroying context.  The application
 * should either call smtp_destroy_session() or call smtp_auth_set_context()
 * with context set to %NULL before doing so.  The authentication API is
 * described separately.
 */

/**
 * smtp_auth_set_context() - Set authentication context.
 * @session: The session.
 * @context: The auth context.
 *
 * Enable the SMTP AUTH verb if @context is not %NULL or disable it when
 * @context is %NULL.  @context must be obtained from the SASL (RFC 4422)
 * client library API defined in ``auth-client.h``.
 *
 * Returns: Non zero on success, zero on failure.
 */
int
smtp_auth_set_context (smtp_session_t session, auth_context_t context)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->auth_context = context;
  return 1;
}

struct mechanism
  {
    struct mechanism *next;
    char *name;
  };

void
destroy_auth_mechanisms (smtp_session_t session)
{
  struct mechanism *mech, *next;

  for (mech = session->auth_mechanisms; mech != NULL; mech = next)
    {
      next = mech->next;
      if (mech->name != NULL)	/* or assert (mech->name != NULL); */
	free (mech->name);
      free (mech);
    }
  session->current_mechanism = session->auth_mechanisms = NULL;
}

void
set_auth_mechanisms (smtp_session_t session, const char *mechanisms)
{
  char buf[64];
  struct mechanism *mech;

  while (read_atom (skipblank (mechanisms), &mechanisms, buf, sizeof buf))
    {
      /* scan existing list to avoid duplicates */
      for (mech = session->auth_mechanisms; mech != NULL; mech = mech->next)
        if (strcasecmp (buf, mech->name) == 0)
          break;
      if (mech != NULL)
        continue;

      /* new mechanism, so add to the list */
      mech = malloc (sizeof (struct mechanism));
      if (mech == NULL)
        {
          /* FIXME: propagate ENOMEM to app. */
          continue;
        }
      mech->name = strdup (buf);
      if (mech->name == NULL)
        {
          /* FIXME: propagate ENOMEM to app. */
          free (mech);
          continue;
        }
      APPEND_LIST (session->auth_mechanisms, session->current_mechanism, mech);
    }
}

int
select_auth_mechanism (smtp_session_t session)
{
  if (session->authenticated)
    return 0;
  if (session->auth_context == NULL)
    return 0;
  if (!auth_client_enabled (session->auth_context))
    return 0;
  /* find the first usable auth mechanism */
  for (session->current_mechanism = session->auth_mechanisms;
       session->current_mechanism != NULL;
       session->current_mechanism = session->current_mechanism->next)
    if (auth_set_mechanism (session->auth_context,
			    session->current_mechanism->name))
      return 1;
  return 0;
}

static int
next_auth_mechanism (smtp_session_t session)
{
  /* find the next usable auth mechanism */
  while ((session->current_mechanism = session->current_mechanism->next) != NULL)
    if (auth_set_mechanism (session->auth_context,
			    session->current_mechanism->name))
      return 1;
  return 0;
}

void
cmd_auth (siobuf_t conn, smtp_session_t session)
{
  char buf[2048];
  const char *response;
  int len;

  assert (session != NULL && session->auth_context != NULL);

  sio_printf (conn, "AUTH %s", auth_mechanism_name (session->auth_context));

  /* Ask SASL for the initial response (if there is one). */
  response = auth_response (session->auth_context, NULL, &len);
  if (response != NULL)
    {
      /* Encode the response and send it back to the server */
      len = b64_encode (buf, sizeof buf, response, len);
      if (len == 0)
	sio_write (conn, " =", 2);
      else if (len > 0)
	{
	  sio_write (conn, " ", 1);
	  sio_write (conn, buf, len);
	}
    }

  sio_write (conn, "\r\n", 2);
  session->cmd_state = -1;
}

void
rsp_auth (siobuf_t conn, smtp_session_t session)
{
  int code;

  code = read_smtp_response (conn, session, &session->mta_status, NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }
  if (code == 4 || code == 5)
    {
      /* If auth mechanism is too weak or encryption is required, give up.
         Otherwise try the next mechanism.  */
      if (session->mta_status.code == 534 || session->mta_status.code == 538)
	session->rsp_state = S_quit;
      else
        {
	  /* If another mechanism cannot be selected, move on to the
	     mail command since the MTA is required to accept mail for
	     its own domain. */
	  if (next_auth_mechanism (session))
	    session->rsp_state = S_auth;
#ifdef USE_ETRN
	  else if (check_etrn (session))
	    session->rsp_state = S_etrn;
#endif
	  else
	    session->rsp_state = initial_transaction_state (session);
        }
    }
  else if (code == 2)
    {
      session->authenticated = 1;
      if (auth_get_ssf (session->auth_context) != 0)
        {
	  /* Add SASL mechanism's encoder/decoder to siobuf.  This is
	     done now, since subsequent commands and responses will be
	     encoded and remain so until the connection to the server
	     is closed.  Auth_encode/decode() call through to the
	     mechanism's coders. */
	  sio_set_securitycb (conn, auth_encode, auth_decode,
	  		      session->auth_context);
	  session->auth_context = NULL;		/* Don't permit AUTH */
	  session->extensions = 0;		/* Turn off extensions */
	  session->rsp_state = S_ehlo;		/* Restart protocol */
        }
#ifdef USE_ETRN
      else if (check_etrn (session))
	session->rsp_state = S_etrn;
#endif
      else
	session->rsp_state = initial_transaction_state (session);
    }
  else if (code == 3)
    session->rsp_state = S_auth2;
  else
    {
      set_error (SMTP_ERR_INVALID_RESPONSE_STATUS);
      session->rsp_state = S_quit;
    }
}

void
cmd_auth2 (siobuf_t conn, smtp_session_t session)
{
  char buf[2048];
  const char *response;
  int len;

  /* Decode the text from the server to get the challenge. */
  len = b64_decode (buf, sizeof buf, session->mta_status.text, -1);
  if (len >= 0)
    {
      /* Send it through SASL and get the response. */
      response = auth_response (session->auth_context, buf, &len);

      /* Encode the response and send it back to the server */
      len = (response != NULL) ? b64_encode (buf, sizeof buf, response, len)
			       : -1;
    }

  /* Abort the AUTH command if base 64 encode/decode fails. */
  if (len < 0)
    sio_write (conn, "*\r\n", 3);
  else
    {
      if (len > 0)
	sio_write (conn, buf, len);
      sio_write (conn, "\r\n", 2);
    }
  session->cmd_state = -1;
}

void
rsp_auth2 (siobuf_t conn, smtp_session_t session)
{
  rsp_auth (conn, session);
}
