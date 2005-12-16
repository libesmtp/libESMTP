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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <missing.h> /* declarations for missing library functions */

#include <errno.h>
#include "api.h"
#include "libesmtp-private.h"
#include "headers.h"

/* This file contains the SMTP client library's external API.  For the
   most part, it just sanity checks function arguments and either carries
   out the simple stuff directly, or passes complicated stuff into the
   bowels of the library and RFC hell.
 */

smtp_session_t
smtp_create_session (void)
{
  smtp_session_t session;

  if ((session = malloc (sizeof (struct smtp_session))) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }

  memset (session, 0, sizeof (struct smtp_session));

  /* Set the default timeouts to the minimum values described in RFC 2822 */
  session->greeting_timeout = GREETING_DEFAULT;
  session->envelope_timeout = ENVELOPE_DEFAULT;
  session->data_timeout = DATA_DEFAULT;
  session->transfer_timeout = TRANSFER_DEFAULT;
  session->data2_timeout = DATA2_DEFAULT;

  return session;
}

int
smtp_set_server (smtp_session_t session, const char *hostport)
{
  char *host, *service;

  SMTPAPI_CHECK_ARGS (session != NULL && hostport != NULL, 0);

  if (session->host != NULL)
    {
      free (session->host);
      session->port = session->host = NULL;
    }

  if ((host = strdup (hostport)) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }

  if ((service = strchr (host, ':')) != NULL)
    *service++ = '\0';

  if (service == NULL)
    session->port = "587";
  else
    session->port = service;
  session->host = host;
  return 1;
}

int
smtp_set_hostname (smtp_session_t session, const char *hostname)
{
#ifdef HAVE_GETHOSTNAME
  SMTPAPI_CHECK_ARGS (session != NULL, 0);
#else
  SMTPAPI_CHECK_ARGS (session != NULL && hostname != NULL, 0);
#endif

  if (session->localhost != NULL)
    free (session->localhost);
#ifdef HAVE_GETHOSTNAME
  if (hostname == NULL)
    {
      session->localhost = NULL;
      return 1;
    }
#endif
  session->localhost = strdup (hostname);
  if (session->localhost == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }
  return 1;
}

smtp_message_t
smtp_add_message (smtp_session_t session)
{
  smtp_message_t message;

  SMTPAPI_CHECK_ARGS (session != NULL, NULL);

  if ((message = malloc (sizeof (struct smtp_message))) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }

  memset (message, 0, sizeof (struct smtp_message));
  message->session = session;
  APPEND_LIST (session->messages, session->end_messages, message);
  return message;
}

int
smtp_enumerate_messages (smtp_session_t session,
			 smtp_enumerate_messagecb_t cb, void *arg)
{
  smtp_message_t message;

  SMTPAPI_CHECK_ARGS (session != NULL && cb != NULL, 0);

  for (message = session->messages; message != NULL; message = message->next)
    (*cb) (message, arg);
  return 1;
}

const smtp_status_t *
smtp_message_transfer_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  return &message->message_status;
}

int
smtp_set_reverse_path (smtp_message_t message, const char *mailbox)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  if (message->reverse_path_mailbox != NULL)
    free (message->reverse_path_mailbox);
  if (mailbox == NULL)
    message->reverse_path_mailbox = NULL;
  else if ((message->reverse_path_mailbox = strdup (mailbox)) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }
  return 1;
}

const smtp_status_t *
smtp_reverse_path_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  return &message->reverse_path_status;
}

int
smtp_message_reset_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  reset_status (&message->reverse_path_status);
  reset_status (&message->message_status);
  return 1;
}

smtp_recipient_t
smtp_add_recipient (smtp_message_t message, const char *mailbox)
{
  smtp_recipient_t recipient;

  SMTPAPI_CHECK_ARGS (message != NULL && mailbox != NULL, NULL);

  if ((recipient = malloc (sizeof (struct smtp_recipient))) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }

  memset (recipient, 0, sizeof (struct smtp_recipient));
  recipient->message = message;
  recipient->mailbox = strdup (mailbox);
  if (recipient->mailbox == NULL)
    {
      free (recipient);
      set_errno (ENOMEM);
      return 0;
    }

  APPEND_LIST (message->recipients, message->end_recipients, recipient);
  return recipient;
}

int
smtp_enumerate_recipients (smtp_message_t message,
			   smtp_enumerate_recipientcb_t cb, void *arg)
{
  smtp_recipient_t recipient;

  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  for (recipient = message->recipients;
       recipient != NULL;
       recipient = recipient->next)
    (*cb) (recipient, recipient->mailbox, arg);
  return 1;
}

const smtp_status_t *
smtp_recipient_status (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, NULL);

  return &recipient->status;
}

int
smtp_recipient_check_complete (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  return recipient->complete;
}

int
smtp_recipient_reset_status (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  reset_status (&recipient->status);
  recipient->complete = 0;
  return 1;
}

/* DSN (RFC 1891) */
int
smtp_dsn_set_ret (smtp_message_t message, enum ret_flags flags)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  message->dsn_ret = flags;
  if (flags != Ret_NOTSET)
    message->session->required_extensions |= EXT_DSN;
  return 1;
}

int
smtp_dsn_set_envid (smtp_message_t message, const char *envid)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  message->dsn_envid = strdup (envid);
  if (message->dsn_envid == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }
  message->session->required_extensions |= EXT_DSN;
  return 1;
}

int
smtp_dsn_set_notify (smtp_recipient_t recipient, enum notify_flags flags)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  recipient->dsn_notify = flags;
  if (flags != Notify_NOTSET)
    recipient->message->session->required_extensions |= EXT_DSN;
  return 1;
}

int
smtp_dsn_set_orcpt (smtp_recipient_t recipient,
		    const char *address_type, const char *address)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  recipient->dsn_addrtype = strdup (address_type);
  if (recipient->dsn_addrtype == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }
  recipient->dsn_orcpt = strdup (address);
  if (recipient->dsn_orcpt == NULL)
    {
      free (recipient->dsn_addrtype);
      set_errno (ENOMEM);
      return 0;
    }
  recipient->message->session->required_extensions |= EXT_DSN;
  return 1;
}

/* SIZE (RFC 1870) */
int
smtp_size_set_estimate (smtp_message_t message, unsigned long size)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  message->size_estimate = size;
  return 1;
}

/* 8BITMIME (RFC 1652) */
int
smtp_8bitmime_set_body (smtp_message_t message, enum e8bitmime_body body)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);
#ifndef USE_CHUNKING
  SMTPAPI_CHECK_ARGS (body != E8bitmime_BINARYMIME, 0);
#endif

  message->e8bitmime = body;
#ifdef USE_CHUNKING
  if (body == E8bitmime_BINARYMIME)
    message->session->required_extensions |= (EXT_BINARYMIME | EXT_CHUNKING);
  else
#endif
  if (body != E8bitmime_NOTSET)
    message->session->required_extensions |= EXT_8BITMIME;
  return 1;
}

/* DELIVERBY (RFC 2852) */
int
smtp_deliverby_set_mode (smtp_message_t message,
			 long time, enum by_mode mode, int trace)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);
  SMTPAPI_CHECK_ARGS ((-999999999 <= time && time <= +999999999), 0);
  SMTPAPI_CHECK_ARGS (!(mode == By_RETURN && time <= 0), 0);

  message->by_time = time;
  message->by_mode = mode;
  message->by_trace = !!trace;
  return 1;
}

int
smtp_set_messagecb (smtp_message_t message, smtp_messagecb_t cb, void *arg)
{
  SMTPAPI_CHECK_ARGS (message != NULL && cb != NULL, 0);

  message->cb = cb;
  message->cb_arg = arg;
  return 1;
}

int
smtp_set_eventcb (smtp_session_t session, smtp_eventcb_t cb, void *arg)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->event_cb = cb;
  session->event_cb_arg = arg;
  return 1;
}

int
smtp_set_monitorcb (smtp_session_t session, smtp_monitorcb_t cb, void *arg,
		    int headers)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->monitor_cb = cb;
  session->monitor_cb_arg = arg;
  session->monitor_cb_headers = headers;
  return 1;
}

int
smtp_start_session (smtp_session_t session)
{
  smtp_message_t message;

  SMTPAPI_CHECK_ARGS (session != NULL && session->host != NULL, 0);
#ifndef HAVE_GETHOSTNAME
  SMTPAPI_CHECK_ARGS (session->localhost != NULL, 0);
#endif

  /* Check that every message has a callback set */
  for (message = session->messages; message != NULL; message = message->next)
    if (message->cb == NULL)
      {
        set_error (SMTP_ERR_INVAL);
        return 0;
      }

  return do_session (session);
}

int
smtp_destroy_session (smtp_session_t session)
{
  smtp_message_t message;
  smtp_recipient_t recipient;

  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  reset_status (&session->mta_status);
  destroy_auth_mechanisms (session);
#ifdef USE_ETRN
  destroy_etrn_nodes (session);
#endif

  if (session->host != NULL)
    free (session->host);
  if (session->localhost != NULL)
    free (session->localhost);

  if (session->msg_source != NULL)
    msg_source_destroy (session->msg_source);

  while (session->messages != NULL)
    {
      message = session->messages->next;

      reset_status (&session->messages->message_status);
      reset_status (&session->messages->reverse_path_status);
      free (session->messages->reverse_path_mailbox);

      while (session->messages->recipients != NULL)
        {
	  recipient = session->messages->recipients->next;

	  reset_status (&session->messages->recipients->status);
	  free (session->messages->recipients->mailbox);

	  if (session->messages->recipients->dsn_addrtype != NULL)
	    free (session->messages->recipients->dsn_addrtype);
	  if (session->messages->recipients->dsn_orcpt != NULL)
	    free (session->messages->recipients->dsn_orcpt);

	  free (session->messages->recipients);
	  session->messages->recipients = recipient;
        }

      destroy_header_table (session->messages);

      if (session->messages->dsn_envid != NULL)
	free (session->messages->dsn_envid);

      free (session->messages);
      session->messages = message;
    }

  free (session);
  return 1;
}

void *
smtp_set_application_data (smtp_session_t session, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  old = session->application_data;
  session->application_data = data;
  return old;
}

void *
smtp_get_application_data (smtp_session_t session)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  return session->application_data;
}

void *
smtp_message_set_application_data (smtp_message_t message, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  old = message->application_data;
  message->application_data = data;
  return old;
}

void *
smtp_message_get_application_data (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  return message->application_data;
}

void *
smtp_recipient_set_application_data (smtp_recipient_t recipient, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  old = recipient->application_data;
  recipient->application_data = data;
  return old;
}

void *
smtp_recipient_get_application_data (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  return recipient->application_data;
}

int
smtp_version (void *buf, size_t len, int what)
{
  static const char version[] = VERSION;

  SMTPAPI_CHECK_ARGS (buf != NULL && len > 0, 0);
  SMTPAPI_CHECK_ARGS (what == 0, 0);

  if (len < sizeof version)
    {
      set_error (SMTP_ERR_INVAL);
      return 0;
    }
  memcpy (buf, version, sizeof version);
  return 1;
}

/* Some applications can't handle one recipient from many failing
   particularly well.  If the 'require_all_recipients' option is
   set, this will fail the entire transaction even if some of the
   recipients were accepted in the RCPT commands. */
int
smtp_option_require_all_recipients (smtp_session_t session, int state)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->require_all_recipients = !!state;
  return 1;
}

/* Set the timeouts.  An absolute minumum timeout of one second is imposed.
   Unless overriden using the OVERRIDE_RFC2822_MINIMUM flag, the minimum
   values recommended in RFC 2822 are enforced.  Return value is the actual
   timeout set or zero on error. */
long
smtp_set_timeout (smtp_session_t session, int which, long value)
{
  long minimum = 1000L;
  int override = 0;

  /* ``which'' is checked in the switch below */
  SMTPAPI_CHECK_ARGS (session != NULL && value > 0, 0);

  override = which & Timeout_OVERRIDE_RFC2822_MINIMUM;
  if (override)
    which &= ~Timeout_OVERRIDE_RFC2822_MINIMUM;
  else
    switch (which)
      {
      case Timeout_GREETING:
	minimum = GREETING_DEFAULT;
	break;
      case Timeout_ENVELOPE:
	minimum = ENVELOPE_DEFAULT;
	break;
      case Timeout_DATA:
	minimum = DATA_DEFAULT;
	break;
      case Timeout_TRANSFER:
	minimum = TRANSFER_DEFAULT;
	break;
      case Timeout_DATA2:
	minimum = DATA2_DEFAULT;
	break;
      }

  if (value < minimum)
    value = minimum;
  switch (which)
    {
    case Timeout_GREETING:
      session->greeting_timeout = value;
      break;
    case Timeout_ENVELOPE:
      session->envelope_timeout = value;
      break;
    case Timeout_DATA:
      session->data_timeout = value;
      break;
    case Timeout_TRANSFER:
      session->transfer_timeout = value;
      break;
    case Timeout_DATA2:
      session->data2_timeout = value;
      break;
    default:
      set_error (SMTP_ERR_INVAL);
      return 0L;
    }

  return value;
}
