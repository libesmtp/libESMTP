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

/**
 * SECTION: libesmtp
 * @title: libESMTP API Reference
 * @section_id:
 *
 * This document describes the libESMTP programming interface.  The libESMTP
 * API is intended for use as an ESMTP client within a Mail User Agent (MUA) or
 * other program that wishes to submit mail to a preconfigured Message
 * Submission Agent (MSA).
 *
 * ## Important
 *
 * This document is believed to be accurate but may not necessarily reflect
 * actual behaviour; to quote *The Hitch Hikers' Guide to the Galaxy*:
 *
 * > Much of it is apocryphal or, at the very least, wildly inaccurate.
 * > However, where it is inaccurate, it is **definitively** inaccurate.
 *
 * If in doubt, consult the source.
 *
 * # Introduction
 *
 * For the most part, the libESMTP API is a relatively thin layer over SMTP
 * protocol operations, that is, most API functions and their arguments are
 * similar to the corresponding protocol commands.  Further API functions
 * manage a callback mechanism which is used to read messages from the
 * application and to report protocol progress to the application.  The
 * remainder of the API is devoted to reporting on a completed SMTP session.
 *
 * Although the API closely models the protocol itself, libESMTP relieves the
 * programmer of all the work needed to properly implement RFC 5321 (formerly
 * RFC2821, formerly RFC 821) and avoids many of the pitfalls in typical SMTP
 * implementations.  It constructs SMTP commands, parses responses, provides
 * socket buffering and pipelining, where appropriate provides for TLS
 * connections and provides a failover mechanism based on DNS allowing multiple
 * redundant MSAs.  Furthermore, support for the SMTP extension mechanism is
 * incorporated by design rather than as an afterthought.
 *
 * There is limited support for processing RFC 5322 message headers.  This is
 * intended to ensure that messages copied to the SMTP server have their
 * correct complement of headers.  Headers that should not be present are
 * stripped and reasonable defaults are provided for missing headers.  In
 * addition, the header API allows the defaults to be tuned and provides a
 * mechanism to specify message headers when this might be difficult to do
 * directly in the message data.
 *
 * libESMTP **does not implement MIME [RFC 2045]** since MIME is, in the words
 * of RFC 2045, *orthogonal* to RFC 5322.  The developer is expected to use a
 * separate library to construct MIME documents or the application should
 * construct them directly.  libESMTP ensures that top level MIME headers are
 * passed unaltered and the header API functions are guaranteed to fail if any
 * header in the name space reserved for MIME is specified, thus ensuring that
 * MIME documents are not accidentally corrupted.
 *
 * ## API
 *
 * The libESMTP API is a relatively small and lightweight interface to the SMTP
 * protocol and its extensions. Internal structures are opaque to the
 * application accessible only through API calls. The majority of the API is
 * used to define the messages and recipients to be transferred to the SMTP
 * server during the protocol session.  Similarly a number of functions are
 * used to query the status of the transfer after the event.  The entire SMTP
 * protocol session is performed by a single function call.
 *
 * ## Reference
 *
 * To use the libESMTP API, include `libesmtp.h`.  Declarations for deprecated
 * symbols must be requested explicitly; define the macro
 * `LIBESMTP_ENABLE_DEPRECATED_SYMBOLS` to be non-zero before including
 * `libesmtp.h`.
 *
 * Internally libESMTP creates and maintains structures to track the state of
 * an SMTP protocol session.  Opaque pointers to these structures are passed
 * back to the application by the API and must be supplied in various other API
 * calls.  The API provides for a callback functions or a simple event
 * reporting mechanism as appropriate so that the application can provide data
 * to libESMTP or track the session's progress.  Further API functions allow
 * the session status to be queried after the event.  The entire SMTP protocol
 * session is performed by only one function call.
 *
 * ## Opaque Pointers
 *
 * All structures and pointers maintained by libESMTP are opaque, that is,
 * the internal detail of libESMTP structures is not made available to the
 * application.  Object oriented programmers may wish to regard the pointers
 * as instances of private classes within libESMTP.
 *
 * Three pointer types are declared as follows:
 * ```c
 * typedef struct smtp_session *<anchor id="smtp_session_t"/>smtp_session_t;
 * typedef struct smtp_message *<anchor id="smtp_message_t"/>smtp_message_t;
 * typedef struct smtp_recipient *<anchor id="smtp_recipient_t"/>smtp_recipient_t;
 * ```
 *
 * ## Thread Safety
 *
 * LibESMTP is thread-aware, however the application is responsible for
 * observing the restrictions below to ensure full thread safety.
 *
 * Do not access a #smtp_session_t, #smtp_message_t or #smtp_recipient_t from
 * more than one thread at a time.  A mutex can be used to protect API calls if
 * the application logic cannot guarantee this.  It is especially important to
 * observe this restriction during a call to smtp_start_session().
 *
 * ## Signal Handling
 *
 * It is advisable for your application to catch or ignore SIGPIPE.  libESMTP
 * sets timeouts as it progresses through the protocol.  In addition the remote
 * server might close its socket at any time.  Consequently libESMTP may
 * sometimes try to write to a socket with no reader.  Catching or ignoring
 * SIGPIPE ensures the application isn't killed accidentally when this happens
 * during the protocol session.
 *
 * Code similar to the following may be used to do this:
 * ```c
 * #include <signal.h>
 *
 * void
 * ignore_sigpipe (void)
 * {
 *   struct sigaction sa;
 *
 *   sa.sa_handler = SIG_IGN;
 *   sigemptyset (&sa.sa_mask);
 *   sa.sa_flags = 0;
 *   sigaction (SIGPIPE, &sa, NULL);
 * }
 * ```
 */

/* This file contains the SMTP client library's external API.  For the
   most part, it just sanity checks function arguments and either carries
   out the simple stuff directly, or passes complicated stuff into the
   bowels of the library and RFC hell.
 */

/**
 * smtp_create_session:
 *
 * Create a descriptor which maintains internal state for the SMTP session.
 *
 * Returns: (transfer full): A #smtp_session_t for the SMTP session or %NULL on
 * failure.
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
 * smtp_set_server:
 * @session: An #smtp_session_t
 * @hostport: Hostname and port (service) for the SMTP server.
 *
 * Set the host name and service for the client connection.  This is specified
 * in the format `host.example.org[:service]` with no whitespace surrounding
 * the colon if `service` is specified.  `service` may be a name from
 * `/etc/services` or a decimal port number.  If not specified the port
 * defaults to 587. Host and service name validity is not checked until an
 * attempt to connect to the remote host.
 *
 * Returns: Zero on failure, non-zero on success.
 */
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

/**
 * smtp_set_hostname:
 * @session: An #smtp_session_t
 * @hostname: The local hostname.
 *
 * Set the name of the localhost.  If `hostname` is %NULL, the local host name
 * will be determined using uname().  If the system does not provide uname() or
 * equivalent, the `hostname` parameter may not be %NULL.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_add_message:
 * @session: An #smtp_session_t
 *
 * Add a message to the list of messages to be transferred to the remote MTA
 * during an SMTP session.
 *
 * Returns: (transfer none): The descriptor for the message state, or %NULL on
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
 * smtp_enumerate_messagecb_t:
 * @message: An #smtp_message_t
 * @arg: User data passed to smtp_enumerate_messages().
 *
 * Callback function to handle a message.
 */

/**
 * smtp_enumerate_messages:
 * @cb:	callback function
 * @arg: user data passed to the callback
 *
 * Call the callback function once for each message in an smtp session.  The
 * arg parameter is passed back to the application via the callback's parameter
 * of the same name.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_message_transfer_status:
 * @message: An #smtp_message_t
 *
 * Retrieve the message transfer success/failure status from a previous SMTP
 * session. This includes SMTP status codes, RFC 2034 enhanced status codes, if
 * available, and text from the server describing the status.  If a message is
 * marked with a success or permanent failure status, it will not be resent if
 * smtp_start_session() is called again.
 *
 * Returns: (transfer none): %NULL if no status information is available,
 * otherwise a pointer to the status information.  The pointer remains valid
 * until the next call to libESMTP in the same thread.
 */
const smtp_status_t *
smtp_message_transfer_status (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, NULL);

  return &message->message_status;
}

/**
 * smtp_set_reverse_path:
 * @message: An #smtp_message_t
 * @mailbox: Reverse path mailbox.
 *
 * Set the reverse path (envelope sender) mailbox address.  @mailbox must be an
 * address using the syntax specified in RFC 5321.  If a null reverse path is
 * required, specify @mailbox as %NULL or `""`.
 *
 * The reverse path mailbox address is used to generate a `From:` header when
 * the message neither contains a `From:` header nor has one been specified
 * using smtp_set_header().
 *
 * It is strongly reccommended that the message supplies a From: header
 * specifying a single mailbox or a Sender: header and a From: header
 * specifying multiple mailboxes or that the libESMTP header APIs are used to
 * create them.
 *
 * Not calling this API has the same effect as specifing @mailbox as %NULL.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_reverse_path_status:
 * @message: An #smtp_message_t
 *
 * Retrieve the reverse path status from a previous SMTP session.  This
 * includes SMTP status codes, RFC 2034 enhanced status codes, if available,
 * and text from the server describing the status.
 *
 * Returns: (transfer none): %NULL if no status information is available,
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
 * smtp_message_reset_status:
 * @message: An #smtp_message_t
 *
 * Reset the message status to the state it would have before
 * smtp_start_session() is called for the first time on the containing session.
 * This may be used to force libESMTP to resend certain messages.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_add_recipient:
 * @message: An #smtp_message_t
 * @mailbox: Recipient mailbox address.
 *
 * Add a recipient to the message.  @mailbox must be an address using the
 * syntax specified in RFC 5321.
 *
 * If neither the message contains a `To:` header nor a `To:` is specified
 * using smtp_set_header(), one header will be automatically generated using
 * the list of envelope recipients.
 *
 * It is strongly reccommended that the message supplies `To:`, `Cc:` and
 * `Bcc:` headers or that the libESMTP header APIs are used to create them.
 *
 * The envelope recipient need not be related to the To/Cc/Bcc recipients, for
 * example, when a mail is resent to the recipients of a mailing list or as a
 * result of alias expansion.
 *
 * Returns: (transfer none): The descriptor for the recipient state or %NULL
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
 * smtp_enumerate_recipientcb_t:
 * @recipient: An #smtp_recipient_t
 * @mailbox: Mailbox name
 * @arg: User data passed to smtp_enumerate_recipients().
 *
 * Callback to process a recipient.
 */

/**
 * smtp_enumerate_recipients:
 * @message: An #smtp_message_t
 * @cb: Callback function to process recipient.
 * @arg: User data passed to the callback.
 *
 * Call the callback function once for each recipient in the SMTP message.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_recipient_status:
 * @recipient: An #smtp_recipient_t
 *
 * Retrieve the recipient success/failure status from a previous SMTP session.
 * This includes SMTP status codes, RFC 2034 enhanced status codes, if
 * available and text from the server describing the status.  If a recipient is
 * marked with a success or permanent failure status, it will not be resent if
 * smtp_start_session() is called again, however it may be used when generating
 * `To:` or `Cc:` headers if required.
 *
 * Returns: (transfer none): %NULL if no status information is available,
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
 * smtp_recipient_check_complete:
 * @recipient: An #smtp_recipient_t
 *
 * Check whether processing is complete for the specified recipient of the
 * message.  Processing is considered complete when an MTA has assumed
 * responsibility for delivering the message, or if it has indicated a
 * permanent failure.
 *
 * Returns: Zero if processing is not complete, non-zero otherwise.
 */
int
smtp_recipient_check_complete (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  return recipient->complete;
}

/**
 * smtp_recipient_reset_status:
 * @recipient: An #smtp_recipient_t
 *
 * Reset the recipient status to the state it would have before
 * smtp_start_session() is called for the first time on the containing session.
 * This is used to force the libESMTP to resend previously successful
 * recipients.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * SECTION: smtp-dsn
 * @section_id:
 * @title: RFC 3461. Delivery Status Notification (DSN)
 *
 * The following APIs implement Delivery Status Notification (DSN) as
 * described in RFC 3461.
 */

/**
 * ret_flags:
 * @Ret_NOTSET: DSN is not requested.
 * @Ret_FULL: Request full message in DSN.
 * @Ret_HDRS: Request only headers in DSN.
 *
 * DSN flags.
 */

/**
 * smtp_dsn_set_ret:
 * @message: An #smtp_message_t
 * @flags: an `enum ret_flags`
 *
 * Instruct the reporting MTA whether to include the full content of the
 * original message in the Delivery Status Notification, or just the headers.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_dsn_set_envid:
 * @message: An #smtp_message_t
 * @envid: Envelope idientifier.
 *
 * Set the envelope identifier.  This value is returned in the
 * DSN and may be used by the MUA to associate the DSN with the
 * message that caused it to be generated.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * notify_flags:
 * @Notify_NOTSET: Notify options not requested.
 * @Notify_NEVER: Never notify.
 * @Notify_SUCCESS: Notify delivery success.
 * @Notify_FAILURE: Notify delivery failure.
 * @Notify_DELAY: Notify delivery is delayed.
 *
 * DSN notify flags.
 */

/**
 * smtp_dsn_set_notify:
 * @recipient: An #smtp_recipient_t
 * @flags: An `enum notify_flags`
 *
 * Set the DSN notify options.  Flags may be %Notify_NOTSET or %Notify_NEVER or
 * the bitwise OR of any combination of %Notify_SUCCESS, %Notify_FAILURE and
 * %Notify_DELAY.
 *
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_dsn_set_orcpt:
 * @recipient: An #smtp_recipient_t
 * @address_type:
 * @address:
 *
 * Set the DSN ORCPT option.
 *
 * Included only for completeness.  This DSN option is only used
 * when performing mailing list expansion or similar situations
 * when the envelope recipient no longer matches the recipient for
 * whom the DSN is to be generated.  Probably only useful to an MTA
 * and should not normally be used by an MUA or other program which
 * submits mail.
 *
 * Returns: Non zero on success, zero on failure.
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
 * SECTION: smtp-size
 * @section_id:
 * @title: SMTP Size Extention.
 *
 * The following APIs implement the SMTP size extention as
 * described in RFC 1870.
 */


/**
 * smtp_size_set_estimate:
 * @message: An #smtp_message_t
 * @size: The size estimate
 *
 * Used by the application to supply an estimate of the size of the
 * message to be transferred.
 *
 * Returns: Non zero on success, zero on failure.
 */
int
smtp_size_set_estimate (smtp_message_t message, unsigned long size)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  message->size_estimate = size;
  return 1;
}

/**
 * SECTION: smtp-8bitmime
 * @section_id:
 * @title: RFC 6152. SMTP 8bit-MIME Transport Extension
 *
 * The 8-bit MIME extension described in RFC 6152 allows an SMTP client to
 * declare the message body is either in strict conformance with RFC 5322 or
 * that it is a MIME document where some or all of the MIME parts use 8-bit
 * encoding.
 */

/**
 * e8bitmime_body:
 * @E8bitmime_NOTSET: Body type not declared.
 * @E8bitmime_7BIT: Body conforms with RFC 5322.
 * @E8bitmime_8BITMIME: Body uses 8 bit encoding.
 *
 * 8BITMIME extension flags.
 */

/**
 * smtp_8bitmime_set_body:
 * @message: An #smtp_message_t
 * @body: Constant from enum #e8bitmime_body
 *
 * Declare the message body conformance.  If the body type is not
 * @E8bitmime_NOTSET, libESMTP will use the event callback to notify the
 * application if the MTA does not support the `8BITMIME` extension.
 *
 * Returns: Non zero on success, zero on failure.
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
 * SECTION: smtp-deliverby
 * @section_id:
 * @title: RFC 2852. SMTP Deliver By Extension
 *
 * # FIXME
 */

/**
 * by_mode:
 * @By_NOTSET: FIXME
 * @By_NOTIFY: FIXME
 * @By_RETURN: FIXME
 *
 * DELIVERBY (RFC 2852) extension flags.
 */

/**
 * smtp_deliverby_set_mode:
 * @message: An #smtp_message_t
 * @time:
 * @mode:
 * @trace:
 *
 * FIXME
 *
 * Returns: Non zero on success, zero on failure.
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

/**
 * smtp_start_session:
 * @session: An #smtp_session_t
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
 * Returns: Zero on failure, non-zero on success.
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
 * smtp_destroy_session:
 * @session: An #smtp_session_t
 *
 * Deallocate all resources associated with the SMTP session.
 *
 * Returns: always returns 1
 */
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
#ifdef USE_TLS
  destroy_starttls_context (session);
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

/**
 * smtp_set_application_data:
 * @session: An #smtp_session_t
 * @data: Application data
 *
 * Associate application data with the #smtp_session_t instance.
 *
 * Returns: Previously set application data or %NULL
 */
void *
smtp_set_application_data (smtp_session_t session, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  old = session->application_data;
  session->application_data = data;
  return old;
}

/**
 * smtp_get_application_data:
 * @session: An #smtp_session_t
 *
 * Get application data from the #smtp_session_t instance.
 *
 * Returns: Previously set application data or %NULL
 */
void *
smtp_get_application_data (smtp_session_t session)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  return session->application_data;
}

/**
 * smtp_message_set_application_data:
 * @message: An #smtp_message_t
 * @data: Application data
 *
 * Associate application data with the #smtp_message_t instance.
 *
 * Returns: Previously set application data or %NULL
 */
void *
smtp_message_set_application_data (smtp_message_t message, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  old = message->application_data;
  message->application_data = data;
  return old;
}

/**
 * smtp_message_get_application_data:
 * @message: An #smtp_message_t
 *
 * Get application data from the #smtp_message_t instance.
 *
 * Returns: Previously set application data or %NULL
 */
void *
smtp_message_get_application_data (smtp_message_t message)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  return message->application_data;
}

/**
 * smtp_recipient_set_application_data:
 * @recipient: An #smtp_recipient_t
 * @data: Application data
 *
 * Associate application data with the #smtp_recipient_t instance.
 *
 * Returns: Previously set application data or %NULL
 */
void *
smtp_recipient_set_application_data (smtp_recipient_t recipient, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  old = recipient->application_data;
  recipient->application_data = data;
  return old;
}

/**
 * smtp_recipient_get_application_data:
 * @recipient: An #smtp_recipient_t
 *
 * Get application data from the #smtp_recipient_t instance.
 *
 * Returns: Previously set application data or %NULL
 */
void *
smtp_recipient_get_application_data (smtp_recipient_t recipient)
{
  SMTPAPI_CHECK_ARGS (recipient != NULL, 0);

  return recipient->application_data;
}

/**
 * smtp_version:
 * @buf: Buffer to receive version string.
 * @len: Length of buffer.
 * @what: Which version information to be retrieved (currently must be 0).
 *
 * Retrieve version information for the libESMTP in use.
 *
 * Returns: Zero on failure, non-zero on success.
 */
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

/**
 * smtp_option_require_all_recipients:
 * @session: An #smtp_session_t
 * @state: Boolean set non-zero to enable.
 *
 * Some applications can't handle one recipient from many failing particularly
 * well.  If the `require_all_recipients` option is set, this will fail the
 * entire transaction even if some of the recipients were accepted in the RCPT
 * commands.
 *
 * Returns: Non zero on success, zero on failure.
 */
int
smtp_option_require_all_recipients (smtp_session_t session, int state)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->require_all_recipients = !!state;
  return 1;
}

/**
 * rfc2822_timeouts:
 * @Timeout_GREETING: Timeout waiting for server greeting.
 * @Timeout_ENVELOPE: Timeout for envelope responses.
 * @Timeout_DATA: Timeout waiting for data transfer to begin.
 * @Timeout_TRANSFER: Timeout for data transfer phase.
 * @Timeout_DATA2: Timeout for ? phase.
 * @Timeout_OVERRIDE_RFC2822_MINIMUM: Bitwise OR with above to override
 * reccommended minimum timeouts.
 *
 * Timeout flags.
 */

/**
 * smtp_set_timeout:
 * @session: An #smtp_session_t
 * @which: Which timeout to set one of enum #rfc2822_timeouts optionally bitwise
 * ORed with @Timeout_OVERRIDE_RFC2822_MINIMUM.
 * @value: duration of timeout in seconds.
 *
 * Set the timeouts.  An absolute minumum timeout of one second is imposed.
 * Unless overriden using the OVERRIDE_RFC2822_MINIMUM flag, the minimum values
 * recommended in RFC 5322 are enforced.
 *
 * Returns: the actual timeout set or zero on error.
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
