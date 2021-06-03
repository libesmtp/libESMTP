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

/**
 * DOC: libESMTP API Reference
 *
 * Core API
 * --------
 *
 * The following describes the libESMTP core API which implements the
 * functionality covered in RFC 5321 and RFC 5322.
 */

/**
 * smtp_create_session() - Create a libESMTP session.
 * @void: No arguments.
 *
 * Create a descriptor which maintains internal state for the SMTP session.
 *
 * Return: A new SMTP session or %NULL on failure.
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

  /* Set the default timeouts to the minimum values described in RFC 5322 */
  session->greeting_timeout = GREETING_DEFAULT;
  session->envelope_timeout = ENVELOPE_DEFAULT;
  session->data_timeout = DATA_DEFAULT;
  session->transfer_timeout = TRANSFER_DEFAULT;
  session->data2_timeout = DATA2_DEFAULT;

  return session;
}

/**
 * smtp_set_server() - Set host and service for submission MTA.
 * @session: The session.
 * @hostport: Hostname and port (service) for the SMTP server.
 *
 * Set the host name and service for the client connection.  This is specified
 * in the format ``host.example.org[:service]`` with no whitespace surrounding
 * the colon if ``service`` is specified.  ``service`` may be a name from
 * ``/etc/services`` or a decimal port number.  If not specified the port
 * defaults to 587. Host and service name validity is not checked until an
 * attempt to connect to the remote host.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_set_server (smtp_session_t session, const char *hostport)
{
  char *host, *service;

  SMTPAPI_CHECK_ARGS (session != NULL && hostport != NULL, 0);

  if (session->canon != NULL)
    free (session->canon);
  session->canon = NULL;

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

/**
 * smtp_get_server_name() - get submission MTA canonic hostname.
 * @session: The session.
 *
 * Get the canonic host name for the submission MTA.  This is only valid after
 * smtp_start_session() has been called, for example in the event callback
 * after a connection has been established. If the canonic name is not
 * available the host name set in smtp_set_server() is returned instead.
 *
 * Return: MTA hostname or NULL.
 */
const char *
smtp_get_server_name (smtp_session_t session)
{
  SMTPAPI_CHECK_ARGS (session != NULL, NULL);

  return session->canon != NULL ? session->canon : session->host;
}

/**
 * smtp_set_hostname() - Set the local host name.
 * @session: The session.
 * @hostname: The local hostname.
 *
 * Set the name of the localhost.  If ``hostname`` is %NULL, the local host
 * name will be determined using uname().  If the system does not provide
 * uname() or equivalent, the ``hostname`` parameter may not be %NULL.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_set_hostname (smtp_session_t session, const char *hostname)
{
#if HAVE_GETHOSTNAME
  SMTPAPI_CHECK_ARGS (session != NULL, 0);
#else
  SMTPAPI_CHECK_ARGS (session != NULL && hostname != NULL, 0);
#endif

  if (session->localhost != NULL)
    free (session->localhost);
#if HAVE_GETHOSTNAME
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

/**
 * smtp_add_message() - Add a message to the session.
 * @session: The session.
 *
 * Add a message to the list of messages to be transferred to the remote MTA
 * during an SMTP session.
 *
 * Return: The descriptor for the message state, or %NULL on
 * failure.
 */
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

/**
 * smtp_enumerate_messages() - Call function for each message in session.
 * @session: The session.
 * @cb:	callback function
 * @arg: user data passed to the callback
 *
 * Call the callback function once for each message in an smtp session.  The
 * arg parameter is passed back to the application via the callback's parameter
 * of the same name.
 *
 * Return: Zero on failure, non-zero on success.
 */
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

/**
 * smtp_message_transfer_status() - Retrieve message status.
 * @message: The message.
 *
 * Retrieve the message transfer success/failure status from a previous SMTP
 * session. This includes SMTP status codes, RFC 2034 enhanced status codes, if
 * available, and text from the server describing the status.  If a message is
 * marked with a success or permanent failure status, it will not be resent if
 * smtp_start_session() is called again.
 *
 * Return: %NULL if no status information is available, otherwise a pointer to
 * the status information.  The pointer remains valid until the next
 * call to libESMTP in the same thread.
 */
const smtp_status_t *
smtp_message_transfer_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  return &message->message_status;
}

/**
 * smtp_set_reverse_path() - Set reverse path mailbox.
 * @message: The message.
 * @mailbox: Reverse path mailbox.
 *
 * Set the reverse path (envelope sender) mailbox address.  @mailbox must be an
 * address using the syntax specified in RFC 5321.  If a null reverse path is
 * required, specify @mailbox as %NULL or ``""``.
 *
 * The reverse path mailbox address is used to generate a ``From:`` header when
 * the message neither contains a ``From:`` header nor has one been specified
 * using smtp_set_header().
 *
 * It is strongly recommended that the message supplies a From: header
 * specifying a single mailbox or a Sender: header and a From: header
 * specifying multiple mailboxes or that the libESMTP header APIs are used to
 * create them.
 *
 * Not calling this API has the same effect as specifing @mailbox as %NULL.
 *
 * Return: Zero on failure, non-zero on success.
 */
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

/**
 * smtp_reverse_path_status() - Get the reverse path status.
 * @message: The message.
 *
 * Retrieve the reverse path status from a previous SMTP session.  This
 * includes SMTP status codes, RFC 2034 enhanced status codes, if available,
 * and text from the server describing the status.
 *
 * Return: %NULL if no status information is available,
 * otherwise a pointer to the status information.  The pointer remains valid
 * until the next call to libESMTP in the same thread.
 */
const smtp_status_t *
smtp_reverse_path_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  return &message->reverse_path_status;
}

/**
 * smtp_message_reset_status() - Reset message status.
 * @message: The message.
 *
 * Reset the message status to the state it would have before
 * smtp_start_session() is called for the first time on the containing session.
 * This may be used to force libESMTP to resend certain messages.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_message_reset_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  reset_status (&message->reverse_path_status);
  reset_status (&message->message_status);
  return 1;
}

/**
 * smtp_add_recipient() - Add a message recipient.
 * @message: The message.
 * @mailbox: Recipient mailbox address.
 *
 * Add a recipient to the message.  @mailbox must be an address using the
 * syntax specified in RFC 5321.
 *
 * If neither the message contains a ``To:`` header nor a ``To:`` is specified
 * using smtp_set_header(), one header will be automatically generated using
 * the list of envelope recipients.
 *
 * It is strongly recommended that the message supplies ``To:``, ``Cc:`` and
 * ``Bcc:`` headers or that the libESMTP header APIs are used to create them.
 *
 * The envelope recipient need not be related to the To/Cc/Bcc recipients, for
 * example, when a mail is resent to the recipients of a mailing list or as a
 * result of alias expansion.
 *
 * Return: The descriptor for the recipient state or %NULL
 * for failure.
 */
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

/**
 * smtp_enumerate_recipients() - Call a function for each recipient.
 * @message: The message.
 * @cb: Callback function to process recipient.
 * @arg: User data passed to the callback.
 *
 * Call the callback function once for each recipient in the SMTP message.
 *
 * Return: Zero on failure, non-zero on success.
 */
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

/**
 * smtp_recipient_status() - Get the recipient status.
 * @recipient: The recipient.
 *
 * Retrieve the recipient success/failure status from a previous SMTP session.
 * This includes SMTP status codes, RFC 2034 enhanced status codes, if
 * available and text from the server describing the status.  If a recipient is
 * marked with a success or permanent failure status, it will not be resent if
 * smtp_start_session() is called again, however it may be used when generating
 * ``To:`` or ``Cc:`` headers if required.
 *
 * Return: %NULL if no status information is available,
 * otherwise a pointer to the status information.  The pointer remains valid
 * until the next call to libESMTP in the same thread.
 */
const smtp_status_t *
smtp_recipient_status (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, NULL);

  return &recipient->status;
}

/**
 * smtp_recipient_check_complete() - Check if recipient processing is completed.
 * @recipient: The recipient.
 *
 * Check whether processing is complete for the specified recipient of the
 * message.  Processing is considered complete when an MTA has assumed
 * responsibility for delivering the message, or if it has indicated a
 * permanent failure.
 *
 * Return: Zero if processing is not complete, non-zero otherwise.
 */
int
smtp_recipient_check_complete (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  return recipient->complete;
}

/**
 * smtp_recipient_reset_status() - Reset recipient status.
 * @recipient: The recipient.
 *
 * Reset the recipient status to the state it would have before
 * smtp_start_session() is called for the first time on the containing session.
 * This is used to force the libESMTP to resend previously successful
 * recipients.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_recipient_reset_status (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  reset_status (&recipient->status);
  recipient->complete = 0;
  return 1;
}

/**
 * DOC: RFC 3461.
 *
 * Delivery Status Notification (DSN)
 * ----------------------------------
 *
 * The following APIs implement Delivery Status Notification (DSN) as
 * described in RFC 3461.
 */


/**
 * smtp_dsn_set_ret() - Set DSN return flags.
 * @message: The message.
 * @flags: an ``enum ret_flags``
 *
 * Instruct the reporting MTA whether to include the full content of the
 * original message in the Delivery Status Notification, or just the headers.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_dsn_set_ret (smtp_message_t message, enum ret_flags flags)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  message->dsn_ret = flags;
  if (flags != Ret_NOTSET)
    message->session->required_extensions |= EXT_DSN;
  return 1;
}

/**
 * smtp_dsn_set_envid() - Set the envelope identifier.
 * @message: The message.
 * @envid: Envelope idientifier.
 *
 * Set the envelope identifier.  This value is returned in the
 * DSN and may be used by the MUA to associate the DSN with the
 * message that caused it to be generated.
 *
 * Return: Zero on failure, non-zero on success.
 */
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


/**
 * smtp_dsn_set_notify() - Set the DSN notify flags.
 * @recipient: The recipient.
 * @flags: An ``enum notify_flags``
 *
 * Set the DSN notify options.  Flags may be %Notify_NOTSET or %Notify_NEVER or
 * the bitwise OR of any combination of %Notify_SUCCESS, %Notify_FAILURE and
 * %Notify_DELAY.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_dsn_set_notify (smtp_recipient_t recipient, enum notify_flags flags)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  recipient->dsn_notify = flags;
  if (flags != Notify_NOTSET)
    recipient->message->session->required_extensions |= EXT_DSN;
  return 1;
}

/**
 * smtp_dsn_set_orcpt() - Set the DSN ORCPT option.
 * @recipient: The recipient.
 * @address_type: String specifying the address type.
 * @address: String specifying the original recipient address.
 *
 * Set the DSN ORCPT option. This DSN option is used only when performing
 * mailing list expansion or similar situations when the envelope recipient no
 * longer matches the recipient for whom the DSN is to be generated.  Probably
 * only useful to an MTA and should not normally be used by an MUA or other
 * program which submits mail.
 *
 * Return: Non zero on success, zero on failure.
 */
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

/**
 * DOC: RFC 1870
 *
 * Size Extention
 * --------------
 *
 * The following APIs implement the SMTP size extention as
 * described in RFC 1870.
 */


/**
 * smtp_size_set_estimate() - Set the message size estimate.
 * @message: The message.
 * @size: The size estimate
 *
 * Used by the application to supply an estimate of the size of the
 * message to be transferred.
 *
 * Return: Non zero on success, zero on failure.
 */
int
smtp_size_set_estimate (smtp_message_t message, unsigned long size)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  message->size_estimate = size;
  return 1;
}

/**
 * DOC: RFC 6152.
 *
 * 8bit-MIME Transport Extension
 * -----------------------------
 *
 * The 8-bit MIME extension described in RFC 6152 allows an SMTP client to
 * declare the message body is either in strict conformance with RFC 5322 or
 * that it is a MIME document where some or all of the MIME parts use 8-bit
 * encoding.
 */


/**
 * smtp_8bitmime_set_body() - Set message body options.
 * @message: The message.
 * @body: Constant from &enum e8bitmime_body.
 *
 * Declare the message body conformance.  If the body type is not
 * %E8bitmime_NOTSET, libESMTP will use the event callback to notify the
 * application if the MTA does not support the ``8BITMIME`` extension.
 *
 * Return: Non zero on success, zero on failure.
 */
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
/**
 * DOC: RFC 2852.
 *
 * Deliver By Extension
 * --------------------
 *
 */


/**
 * smtp_deliverby_set_mode() - Set delivery tracing and conditions.
 * @message: The message.
 * @time: Deliver by specified time.
 * @mode: Delivery mode.
 * @trace: Trace mode.
 *
 * If @time is before the earliest time the server can guarantee delivery,
 * libESMTP will emit a %SMTP_EV_DELIVERBY_EXPIRED event. The application may
 * optionally adjust latest acceptable the delivery time, otherwise the MAIL
 * command will be failed by the server.
 *
 * Return: Non zero on success, zero on failure.
 */
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

/**
 * DOC: Callbacks
 *
 * Callbacks
 * ---------
 *
 */

/**
 * smtp_set_messagecb() - Set message reader.
 * @message: The message.
 * @cb: Callback function.
 * @arg: application data (closure) passed to the callback.
 *
 * Set a callback function to read the message.
 *
 * Return: Non zero on success, zero on failure.
 */
int
smtp_set_messagecb (smtp_message_t message, smtp_messagecb_t cb, void *arg)
{
  SMTPAPI_CHECK_ARGS (message != NULL && cb != NULL, 0);

  message->cb = cb;
  message->cb_arg = arg;
  return 1;
}

/**
 * smtp_set_eventcb() - Set event callback.
 * @session: The session.
 * @cb: Callback function.
 * @arg: application data (closure) passed to the callback.
 *
 * Set callback to be called as protocol events occur.
 *
 * Return: Non zero on success, zero on failure.
 */
int
smtp_set_eventcb (smtp_session_t session, smtp_eventcb_t cb, void *arg)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->event_cb = cb;
  session->event_cb_arg = arg;
  return 1;
}

/**
 * smtp_set_monitorcb() - Set protocol monitor.
 * @session: The session.
 * @cb: Callback function.
 * @arg: application data (closure) passed to the callback.
 * @headers: non-zero to view headers.
 *
 * Set callback to be monitor the SMTP session.  The monitor function is called
 * to with the text of the protocol exchange and is flagged as being from the
 * server or the client.  Because the message body is potentially large it is
 * excluded from the monitor.  However, @headers is non-zero the message
 * headers are monitored.
 *
 * Return: Non zero on success, zero on failure.
 */
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

/**
 * DOC: Protocol
 *
 * SMTP Protocol
 * -------------
 *
 */

/**
 * smtp_start_session() - Start an SMTP session.
 * @session: The session to start.
 *
 * Initiate a mail submission session with an SMTP server.
 *
 * This connects to an SMTP server and transfers the messages in the session.
 * The SMTP envelope is constructed using the message and recipient parameters
 * set up previously.  The message callback is then used to read the message
 * contents to the server.  As the RFC 5322 headers are read from the
 * application, they may be processed.  Header processing terminates when the
 * first line containing only CR-LF is encountered.  The remainder of the
 * message is copied verbatim.
 *
 * This call is atomic in the sense that a connection to the server is made
 * only when this is called and is closed down before it returns, i.e. there is
 * no connection to the server outside this function.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_start_session (smtp_session_t session)
{
  smtp_message_t message;

  SMTPAPI_CHECK_ARGS (session != NULL && session->host != NULL, 0);
#if !HAVE_GETHOSTNAME
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

/**
 * DOC: Teardown
 *
 * Tear Down Session
 * -----------------
 *
 */

/**
 * smtp_destroy_session() - Destroy a libESMTP session.
 * @session: The session.
 *
 * Deallocate all resources associated with the SMTP session.
 *
 * If application data has been set on any of the libESMTP structures, the
 * associated destroy callback will be called with the application data as an
 * argument.
 *
 * If the deprecated legacy APIs are used to set application data or a destroy
 * callback is not provided, the application must explicitly enumerate all
 * messages and their recipients, and retrieve the data to be freed.
 *
 * Return: always returns 1
 */
int
smtp_destroy_session (smtp_session_t session)
{
  smtp_message_t message, next_message;
  smtp_recipient_t recipient, next_recipient;

  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  if (session->application_data != NULL && session->release != NULL)
    (*session->release) (session->application_data);

  reset_status (&session->mta_status);
  destroy_auth_mechanisms (session);
#ifdef USE_ETRN
  destroy_etrn_nodes (session);
#endif
#ifdef USE_TLS
  destroy_starttls_context (session);
#endif

  if (session->canon != NULL)
    free (session->canon);
  if (session->host != NULL)
    free (session->host);
  if (session->localhost != NULL)
    free (session->localhost);

  if (session->msg_source != NULL)
    msg_source_destroy (session->msg_source);

  for (message = session->messages; message != NULL; message = next_message)
    {
      next_message = message->next;

      if (message->application_data != NULL && message->release != NULL)
	(*message->release) (message->application_data);

      reset_status (&message->message_status);
      reset_status (&message->reverse_path_status);
      free (message->reverse_path_mailbox);

      for (recipient = message->recipients;
           recipient != NULL;
      	   recipient = next_recipient)
        {
	  next_recipient = recipient->next;

	  if (recipient->application_data != NULL && recipient->release != NULL)
	    (*recipient->release) (recipient->application_data);

	  reset_status (&recipient->status);
	  free (recipient->mailbox);

	  if (recipient->dsn_addrtype != NULL)
	    free (recipient->dsn_addrtype);
	  if (recipient->dsn_orcpt != NULL)
	    free (recipient->dsn_orcpt);

	  free (recipient);
        }

      destroy_header_table (message);

      if (message->dsn_envid != NULL)
	free (message->dsn_envid);

      free (message);
    }

  free (session);
  return 1;
}

/**
 * DOC: Data
 *
 * Application Data
 * ----------------
 *
 * Applications may attach arbitrary data to any of the libESMTP structures.
 * The _release() variants of the APIs are preferred to the legacy versions as
 * these will call a destroy callback when the smtp_session_t structure is
 * destroyed, helping to avoid memory leaks that might have occurred using the
 * deprecated APIs.
 */

/**
 * smtp_set_application_data() - Associate data with a session.
 * @session: The session.
 * @data: Application data
 *
 * Associate application data with the session.
 *
 * Only available when %LIBESMTP_ENABLE_DEPRECATED_SYMBOLS is defined.
 * Use smtp_set_application_data_release() instead.
 *
 * Return: Previously set application data or %NULL
 */
void *
smtp_set_application_data (smtp_session_t session, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (session != NULL, NULL);

  old = session->application_data;
  session->application_data = data;
  session->release = NULL;
  return old;
}

/**
 * smtp_set_application_data_release() - Associate data with a session.
 * @session: The session.
 * @data: Application data
 * @release: function to free/unref data.
 *
 * Associate application data with the session.
 * If @release is non-NULL it is called to free or unreference data when
 * changed or the session is destroyed.
 */
void
smtp_set_application_data_release (smtp_session_t session, void *data,
                                   void (*release) (void *))
{
  SMTPAPI_CHECK_ARGS (session != NULL, /* void */);

  if (session->application_data != NULL && session->release != NULL)
    (*session->release) (session->application_data);

  session->release = release;
  session->application_data = data;
}

/**
 * smtp_get_application_data() - Get data from a session.
 * @session: The session.
 *
 * Get application data from the session.
 *
 * Return: Previously set application data or %NULL
 */
void *
smtp_get_application_data (smtp_session_t session)
{
  SMTPAPI_CHECK_ARGS (session != NULL, NULL);

  return session->application_data;
}

/**
 * smtp_message_set_application_data() - Associate data with a message.
 * @message: The message.
 * @data: Application data
 *
 * Associate application data with the message.
 *
 * Only available when %LIBESMTP_ENABLE_DEPRECATED_SYMBOLS is defined.
 * Use smtp_message_set_application_data_release() instead.
 *
 * Return: Previously set application data or %NULL
 */
void *
smtp_message_set_application_data (smtp_message_t message, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  old = message->application_data;
  message->application_data = data;
  message->release = NULL;
  return old;
}

/**
 * smtp_message_set_application_data_release() - Associate data with a message.
 * @message: The message.
 * @data: Application data
 * @release: function to free/unref data.
 *
 * Associate application data with the message.
 * If @release is non-NULL it is called to free or unreference data when
 * changed or the message is destroyed.
 */
void
smtp_message_set_application_data_release (smtp_message_t message, void *data,
					   void (*release) (void *))
{
  SMTPAPI_CHECK_ARGS (message != NULL, /* void */);

  if (message->application_data != NULL && message->release != NULL)
    (*message->release) (message->application_data);

  message->release = release;
  message->application_data = data;
}

/**
 * smtp_message_get_application_data() - Get data from a message.
 * @message: The message.
 *
 * Get application data from the message.
 *
 * Return: Previously set application data or %NULL
 */
void *
smtp_message_get_application_data (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  return message->application_data;
}

/**
 * smtp_recipient_set_application_data() - Associate data with a recipient.
 * @recipient: The recipient.
 * @data: Application data
 *
 * Associate application data with the recipient.
 *
 * Only available when %LIBESMTP_ENABLE_DEPRECATED_SYMBOLS is defined.
 * Use smtp_recipient_set_application_data_release() instead.
 *
 * Return: Previously set application data or %NULL
 */
void *
smtp_recipient_set_application_data (smtp_recipient_t recipient, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (recipient != NULL, NULL);

  old = recipient->application_data;
  recipient->application_data = data;
  recipient->release = NULL;
  return old;
}

/**
 * smtp_recipient_set_application_data_release() - Associate data with a recipient.
 * @recipient: The recipient.
 * @data: Application data
 * @release: function to free/unref data.
 *
 * Associate application data with the recipient.
 * If @release is non-NULL it is called to free or unreference data when
 * changed or the recipient is destroyed.
 */
void
smtp_recipient_set_application_data_release (smtp_recipient_t recipient,
					     void *data,
					  void (*release) (void *))
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, /* void */);

  if (recipient->application_data != NULL && recipient->release != NULL)
    (*recipient->release) (recipient->application_data);

  recipient->release = release;
  recipient->application_data = data;
}

/**
 * smtp_recipient_get_application_data() - Get data from a recipient.
 * @recipient: The recipient.
 *
 * Get application data from the recipient.
 *
 * Return: Previously set application data or %NULL
 */
void *
smtp_recipient_get_application_data (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, NULL);

  return recipient->application_data;
}

/**
 * DOC: Miscellaneous
 *
 * Miscellaneous APIs
 * ------------------
 *
 */

/**
 * smtp_version() - Identify libESMTP version.
 * @buf: Buffer to receive version string.
 * @len: Length of buffer.
 * @what: Which version information to be retrieved.
 *
 * Retrieve version information for the libESMTP in use. The version number
 * depends on the @what parameter.  When @what == Ver_VERSION the libESMTP
 * version is returned; when @what == Ver_SO_VERSION the .so file version is
 * returned; when @what == Ver_LT_VERSION the libtool style API version is
 * returned.
 *
 * If the supplied buffer is too small to receive the version number, this
 * function fails.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_version (void *buf, size_t len, int what)
{
  static const char *v;

  SMTPAPI_CHECK_ARGS (buf != NULL && len > 0, 0);
  SMTPAPI_CHECK_ARGS (what >= 0 && what < 3, 0);

  switch (what)
  {
  case Ver_VERSION:
    v = VERSION;
    break;
  case Ver_SO_VERSION:
    v = SO_VERSION;
    break;
  case Ver_LT_VERSION:
    v = LT_VERSION;
    break;
  }

  if (strlcpy (buf, v, len) > len)
    {
      set_error (SMTP_ERR_INVAL);
      return 0;
    }
  return 1;
}

/**
 * smtp_option_require_all_recipients() - FAIL if server rejects any recipient.
 * @session: The session.
 * @state: Boolean set non-zero to enable.
 *
 * Some applications can't handle one recipient from many failing particularly
 * well.  If the ``require_all_recipients`` option is set, this will fail the
 * entire transaction even if some of the recipients were accepted in the RCPT
 * commands.
 *
 * Return: Non zero on success, zero on failure.
 */
int
smtp_option_require_all_recipients (smtp_session_t session, int state)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->require_all_recipients = !!state;
  return 1;
}

/**
 * smtp_set_timeout() - Set session timeouts.
 * @session: The session.
 * @which: Which timeout to set. A constant from &enum rfc2822_timeouts.
 * @value: duration of timeout in seconds.
 *
 * Set the protocol timeouts.  @which is a constant from &enum rfc2822_timeouts
 * specifying which timeout to set.  The minimum values recommended in RFC 5322
 * are enforced unless @which is bitwise-ORed with
 * %Timeout_OVERRIDE_RFC2822_MINIMUM.  An absolute minumum timeout of one
 * second is imposed.
 *
 * Return: the actual timeout set or zero on error.
 */
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
