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

/* The SMTP protocol engine and handler functions for the core SMTP
   commands and their extended parameters.  Extended commands are mostly
   in files of their own. */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include <missing.h> /* declarations for missing library functions */

#include <sys/socket.h>
#if HAVE_LWRES_NETDB_H
# include <lwres/netdb.h>
#else
# include <netdb.h>
#endif

#if HAVE_UNAME
#include <sys/utsname.h>
#endif

#include "libesmtp-private.h"
#include "message-source.h"
#include "siobuf.h"
#include "tokens.h"
#include "headers.h"
#include "protocol.h"

struct protocol_states
  {
    void (*cmd) (siobuf_t conn, smtp_session_t session);
    void (*rsp) (siobuf_t conn, smtp_session_t session);
  };

/* The following array of state handlers is indexed by the state (!)  */
struct protocol_states protocol_states[] =
  {
#define S(x)		{ cmd_##x, rsp_##x, },
#include "protocol-states.h"
  };

static int
set_first_recipient (smtp_session_t session)
{
  smtp_recipient_t recipient;

  if (session->current_message == NULL)
    return 0;
  
  for (recipient = session->current_message->recipients;
       recipient != NULL;
       recipient = recipient->next)
    if (!recipient->complete)
      break;
  session->cmd_recipient = session->rsp_recipient = recipient;
  return recipient != NULL;
}

/* Return a pointer to the next unsent recipient.  This can't operate
   on the session structure directly as the other first/next functions
   do since there are two variables for the current recipient due to
   the implementation of PIPELINING.  */
static smtp_recipient_t
next_recipient (smtp_recipient_t recipient)
{
  while ((recipient = recipient->next) != NULL)
    if (!recipient->complete)
      break;
  return recipient;
}

/* Set the session's current message to the next unsent message.
 */
int
next_message (smtp_session_t session)
{
  while ((session->current_message = session->current_message->next) != NULL)
    if (set_first_recipient (session))
      return 1;
  return 0;
}

/* Set the current message to the first unsent message in the
   session.  */
static int
set_first_message (smtp_session_t session)
{
  for (session->current_message = session->messages;
       session->current_message != NULL;
       session->current_message = session->current_message->next)
    if (set_first_recipient (session))
      return 1;
  return 0;
}

/*****************************************************************************
 * The main protocol engine.
 *****************************************************************************/

int
do_session (smtp_session_t session)
{
  struct addrinfo hints, *res, *addrs;
  int err;
  int sd;
  siobuf_t conn;
  int nresp, status, want_flush, fast;
  char *nodename;

#if HAVE_UNAME
  if (session->localhost == NULL)
    {
      struct utsname name;

      if (uname (&name) < 0)
        {
	  set_errno (errno);
	  return 0;
        }
      session->localhost = strdup (name.nodename);
      if (session->localhost == NULL)
        {
	  set_errno (ENOMEM);
	  return 0;
        }
    }
#elif HAVE_GETHOSTNAME
  if (session->localhost == NULL)
    {
      char host[256];

      if (gethostname (host, sizeof host) < 0)
        {
	  set_errno (errno);
	  return 0;
        }
      session->localhost = strdup (host);
      if (session->localhost == NULL)
        {
	  set_errno (ENOMEM);
	  return 0;
        }
    }
#endif

  /* Initialise the current message and recipient variables in the
     session.  This returns zero if there is no work to do.  */
#ifndef USE_ETRN
  if (!set_first_message (session))
#else
  if (!set_first_message (session) && session->etrn_nodes == NULL)
#endif
    {
      set_error (SMTP_ERR_NOTHING_TO_DO);
      return 0;
    }

  /* Create a message source only if it is needed.
   */
  if (session->msg_source == NULL && session->current_message != NULL)
    {
      session->msg_source = msg_source_create ();
      if (session->msg_source == NULL)
        {
	  set_errno (ENOMEM);
	  return 0;
        }
    }

  /* Connect to the SMTP server. */

  errno = 0;
  nodename = (session->host == NULL || *session->host == '\0') ? NULL
							       : session->host;
  /* Use the RFC 3493/Posix resolver interface.  This allows for much
     cleaner code, protocol independence and thread safety. */
  memset (&hints, 0, sizeof hints);
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  err = getaddrinfo (nodename, session->port, &hints, &res);
  if (err != 0)
    {
      set_herror (err);
      return 0;
    }

  session->canon = res->ai_canonname != NULL ? strdup (res->ai_canonname) : NULL;

  /* Try to establish an SMTP session with each host in turn until one
     succeeds.  */
  for (addrs = res; addrs != NULL; addrs = addrs->ai_next)
    {
      sd = socket (addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
      if (sd < 0)
	{
	  set_errno (errno);
	  continue;
	}
      if (connect (sd, addrs->ai_addr, addrs->ai_addrlen) < 0)
	{
	  /* Failed to connect.  Close the socket and try again.  */
	  set_errno (errno);
	  close (sd);
	  continue;
	}

      /* Add buffering to the socket */
      conn = sio_attach (sd, sd, SIO_BUFSIZE);
      if (conn == NULL)
	{
	  set_errno (ENOMEM);
	  freeaddrinfo (res);
	  close (sd);
	  return 0;
	}

      /* If monitoring the protocol, pass the callback on to the sio_
	 package. */
      if (session->monitor_cb != NULL)
	sio_set_monitorcb (conn, session->monitor_cb, session->monitor_cb_arg);

      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_CONNECT, session->event_cb_arg);

      /* Outer loop of the protocol.  This is trickier than superficial
	 examination of RFC 821 would suggest, however the complexity is
	 required to handle batched commands and responses.  RFC 821 makes
	 no comment on the underlying transport so it cannot be assumed that
	 each command and its response is transported in a single packet on
	 the network or that the network preserves packet boundaries (or,
	 for that matter, that the underlying transport is even a network,
	 e.g. it might be a pipe or a AF_UNIX socket).  Multiple commands
	 may be sent in a single packet and similarly multiple responses
	 may be returned in a single packet. RFC 2920 which describes the
	 PIPELINING SMTP extension clarifies this for the case where the
	 underlying transport is TCP/IP.

	 In this implementation the outer loop will batch together as many
	 commands as possible until the remote server sends a response (this
	 will normally happen after the local transmit buffer is flushed)
	 or a command has been sent which requires a response before the
	 client can proceed.

	 In order not to break certain SMTP server implementations, flushing
	 is done after every command unless the PIPELINING keyword is received
	 in response to the EHLO command.  Even when PIPELINING is in force
	 certain commands *always* flush the transmit buffer.

	 One good reason for wanting PIPELINING is that when submitting mail
	 to an ISP's sluggish server, there will be a big performance boost
	 since many round trips are eliminated.

	 As the protocol engine advances through its states, it will walk
	 through the messages and recipients within the session.

	 The protocol functions set a few timeouts as they progress.  The
	 values set are those recommended in RFC 5321.

	 [is it just me, or does everybody find that it's easier to implement
	  protocols on the server side?]
       */

      /* Reset variables to their initial state before entering the protocol
	 main loop. */
      session->extensions = 0;
      session->try_fallback_server = 0;
      reset_status (&session->mta_status);
      destroy_auth_mechanisms (session);
      session->authenticated = 0;
#ifdef USE_TLS
      session->using_tls = 0;
#endif

      nresp = 0;
      session->cmd_state = session->rsp_state = 0;
      while (session->rsp_state >= 0)
	{
	  if (session->cmd_state == -1)
	    session->cmd_state = session->rsp_state;
	  (*protocol_states[session->cmd_state].cmd) (conn, session);
	  sio_mark (conn);
	  if (!(session->extensions & EXT_PIPELINING))
	    session->cmd_state = -1;
	  nresp++;

	  if (session->rsp_state < 0)
	    break;

	  /* The following loop polls the server and reads or writes
	     to it as required.

	     When the command state is set to -1, this signals that no
	     more commands can be issued until the response to the most
	     recent command has been processed, therefore the write
	     buffer must be explicitly flushed.  If the command state is
	     not -1, more commands may be issued however, pending
	     responses from the server should be processed.  When this
	     is the case, sio_poll should return immediately if there is
	     nothing to read.  `fast' requests this non-blocking poll.

	     `want_flush' indicates that the write buffer should be
	     sent to the server.  This flag remains set until the buffer
	     has been written.

	     After explicitly flushing the buffer, sio_poll blocks
	     waiting to read data from the server since the server may
	     take some time to complete the pending commands.
           */
	  want_flush = (session->cmd_state == -1);
	  fast = (session->cmd_state != -1);
	  while ((status = sio_poll (conn, nresp > 0, want_flush, fast)) > 0)
	    {
	      if (status & SIO_READ)
		{
		  nresp--;

		  /* TODO: change so that the response line is parsed
		           here.  This means that the server 421
		           response which can be issued at any time may
			   be checked for here. */

		  /* When reading from the server in the response state
		     handlers the read call blocks.  Complete responses
	             must be read from the server before processing and
	             an individual response may be larger than the read
	             buffer.  */
		  (*protocol_states[session->rsp_state].rsp) (conn, session);
		}
	      /* XXX - Here I assume that once the write fd becomes
		       available for writing, it stays that way until
		       it is written to.  I.e. a blocking read() or a
		       subsequent poll() will not revoke the writable
		       status.	Could somebody confirm that this is the
		       case? */
	      if (status & SIO_WRITE)
		{
		  sio_flush (conn);
		  want_flush = 0;
		}
	    }
	  if (status < 0)
	    {
	      set_error (SMTP_ERR_DROPPED_CONNECTION);
	      break;
	    }
	}

      sio_detach (conn);
      close (sd);

      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_DISCONNECT,
	                      session->event_cb_arg);

      /* This flag will be set if the server was reached OK but was the
         wrong kind of server or the client is told to go away.  So if
         not set the protocol must have concluded sucessfully. */
      if (!session->try_fallback_server)
	{
	  freeaddrinfo (res);
	  return 1;
        }
    }

  /* If the loop terminated, couldn't work with any servers. */
  freeaddrinfo (res);
  return 0;
}

/*****************************************************************************
 * Response parser.
 *****************************************************************************/

static int
parse_status_triplet (char *p, char **ep, struct smtp_status *triplet)
{
  triplet->enh_class = strtol (p, &p, 10);
  if (*p++ != '.')
    return 0;
  triplet->enh_subject = strtol (p, &p, 10);
  if (*p++ != '.')
    return 0;
  triplet->enh_detail = strtol (p, &p, 10);
  *ep = p;
  return 1;
}

static int
compare_status_triplet (struct smtp_status *a, struct smtp_status *b)
{
  return a->enh_class == b->enh_class
         && a->enh_subject == b->enh_subject
         && a->enh_detail == b->enh_detail;
}

/* Free memory allocated in read_smtp_response()
   and clear the status structure */
void
reset_status (struct smtp_status *status)
{
  if (status->text != NULL)
    free ((void *) status->text);
  memset (status, 0, sizeof (struct smtp_status));
}

/* All SMTP responses have standard syntax.  This function could be
   called by the protocol engine above and the results from the parsed
   response passed to the response handler functions.  However certain
   commands involve extra intermediate exchanges with the server that
   may not correspond to the standard syntax.  The response handlers
   must therefore call this function themselves. */
int
read_smtp_response (siobuf_t conn, smtp_session_t session,
                    struct smtp_status *status,
                    int (*cb) (smtp_session_t, char *))
{
  struct catbuf text;
  char buf[1024];
  char *p, *nul;
  int code, more, want_enhanced, textlen, quit_now;
  struct smtp_status triplet;

  /* First line of an SMTP response is the normal one.  Put it in a buffer.
     Save the status code and the enhanced status triplet if ENHSTATUSCODES
     is set.  The remainder of the line is the message from the server.
     If there are any continuation lines, get the status code and enhanced
     status triplet and check that they are the same as the first response
     line.  If not there is a protocol error.  Process the extra lines
     by calling a callback.  If not supplied, concatenate the lines
     (excluding the status info) with the first line. */

  reset_status (status);
  if ((p = sio_gets (conn, buf, sizeof buf)) == NULL)
    {
      set_error (SMTP_ERR_DROPPED_CONNECTION);
      return -1;
    }
  status->code = strtol (p, &p, 10);
  if (!(*p == ' ' || *p == '-'))
    {
      set_error (SMTP_ERR_INVALID_RESPONSE_SYNTAX);
      return -1;
    }
  more = *p++ == '-';

  /* RFC 2034 states that only 2xx, 4xx and 5xx responses are accompanied
     by enhanced status codes. */
  code = status->code / 100;
  want_enhanced = (session->extensions & EXT_ENHANCEDSTATUSCODES);
  if (code != 2 && code != 4 && code != 5)
    want_enhanced = 0;

  if (want_enhanced && !parse_status_triplet (p, &p, status))
    {
      quit_now = 0;	/* Be tolerant by default */
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_SYNTAXWARNING,
			      session->event_cb_arg, &quit_now);
      if (quit_now)
	{
	  set_error (SMTP_ERR_INVALID_RESPONSE_SYNTAX);
	  return -1;
	}
      want_enhanced = 0;
    }
  while (isspace (*p))
    p++;

  /* p points to the remainder of the line.  This is the text of the
     server message */
  cat_init (&text, 128);
  concatenate (&text, p, -1);

  while (more)
    {
      if ((p = sio_gets (conn, buf, sizeof buf)) == NULL)
	{
	  cat_free (&text);
	  set_error (SMTP_ERR_DROPPED_CONNECTION);
	  return -1;
	}
      code = strtol (p, &p, 10);
      if (code != status->code)
	{
	  cat_free (&text);
	  set_error (SMTP_ERR_STATUS_MISMATCH);
	  return -1;
	}
      if (!(*p == ' ' || *p == '-'))
	{
	  cat_free (&text);
	  set_error (SMTP_ERR_INVALID_RESPONSE_SYNTAX);
	  return -1;
	}
      more = *p++ == '-';
      if (want_enhanced)
	{
	  if (!parse_status_triplet (p, &p, &triplet))
	    {
	      cat_free (&text);
	      set_error (SMTP_ERR_INVALID_RESPONSE_SYNTAX);
	      return -1;
	    }
	  if (!compare_status_triplet (status, &triplet))
	    {
	      cat_free (&text);
	      set_error (SMTP_ERR_STATUS_MISMATCH);
	      return -1;
	    }
	}

      /* Skip whitespace but don't wander over the CRLF */
      while (isspace (*p) && isprint (*p))
	p++;

      /* Check that the line is correctly terminated. */
      nul = strchr (p, '\0');
      if (nul == NULL || nul == p || nul[-1] != '\n')
        {
	  cat_free (&text);
	  set_error (SMTP_ERR_UNTERMINATED_RESPONSE);
	  return -1;
        }

      /* `p' points to the remainder of the line.  Either process with the
         callback or concatenate with the first line. */
      if (cb != NULL)
        (*cb) (session, p);
      else
	concatenate (&text, p, nul - p);

      /* Check if the total text returned in a multiline response
         exceeds 4k.  Abort if this happens, this might be a DoS attack
         or a broken server. */
      textlen = 0;
      cat_buffer (&text, &textlen);
      if (textlen > 4096)
        {
	  cat_free (&text);
	  set_error (SMTP_ERR_UNTERMINATED_RESPONSE);
	  return -1;
        }
    }

  /* Terminate and save the response text */
  concatenate (&text, "", 1);
  status->text = cat_shrink (&text, NULL);

  return status->code / 100;
}

/*****************************************************************************
 * Command and response handlers.  Return value is the next send/response
 * state.  If a cmd_xxxx() function returns -1, the response determines
 * the next state, implying that a flush is needed.
 *****************************************************************************/

/*****************************************************************************
 * Server Greeting
 *****************************************************************************/

/* If the response to the server greeting is not a 2xx status code,
   issue the QUIT command and terminate the session.  */
void
cmd_greeting (siobuf_t conn, smtp_session_t session)
{
  /* Set a five minute timeout. */
  sio_set_timeout (conn, session->greeting_timeout);
  session->cmd_state = -1;
}

void
rsp_greeting (siobuf_t conn, smtp_session_t session)
{
  int code;

  code = read_smtp_response (conn, session, &session->mta_status, NULL);
  if (code == 2 && session->mta_status.code == 220)
    session->rsp_state = S_ehlo;
  else if (code == 4 || code == 5)
    {
      session->rsp_state = S_quit;	/* Graceful exit using QUIT */
      session->try_fallback_server = 1;
    }
  else
    {
      session->rsp_state = -1;		/* Drop the connection and run */
      session->try_fallback_server = 1;
    }

  /* TODO: certain broken servers may break when the EHLO command is
           issued.  For example, one known behaviour is to close the
           connection after returning the 501 response.  This is wrong
           but it happens.

           An API option is needed to avoid using EHLO!
   */
}

/*****************************************************************************
 * EHLO 
 *****************************************************************************/

/* EHLO is the preferred client greeting to the server.  The parameter is
   the FQDN of the client host.  The server response is the greeting line
   followed by extra lines listing the server capabilities.  The additional
   lines are parsed and set the session capability flags.  If the response
   is "501 command not implemented", the HELO command should be used instead.
   Since the next command issued depends on the server response the output
   must be flushed.

   Next state is one of Auth, Helo, Mail.
 */
void
cmd_ehlo (siobuf_t conn, smtp_session_t session)
{
  sio_printf (conn, "EHLO %s\r\n", session->localhost);
  session->cmd_state = -1;
}

#define no_required_extension(s,e)	\
		(((s)->required_extensions & (e)) && !((s)->extensions & (e)))

static int
report_extensions (smtp_session_t session)
{
  int quit_now;
  unsigned long exts = 0;

  /* Report extensions that are required but not available.
   */
  if (no_required_extension (session, EXT_DSN))
    {
      quit_now = 0;
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_EXTNA_DSN,
			      session->event_cb_arg, &quit_now);
      if (quit_now)
        exts |= EXT_DSN;
    }
#ifdef USE_CHUNKING
  if (no_required_extension (session, EXT_CHUNKING))
    {
      quit_now = 0;
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_EXTNA_CHUNKING,
			      session->event_cb_arg, &quit_now);
      if (quit_now)
        exts |= EXT_CHUNKING;
    }
  if (no_required_extension (session, EXT_BINARYMIME))
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_EXTNA_BINARYMIME,
			      session->event_cb_arg);
      exts |= EXT_BINARYMIME;
    }
#endif
  if (no_required_extension (session, EXT_8BITMIME))
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_EXTNA_8BITMIME,
			      session->event_cb_arg);
      exts |= EXT_8BITMIME;
    }
#ifdef USE_ETRN
  if (no_required_extension (session, EXT_ETRN))
    {
      quit_now = 1;
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_EXTNA_ETRN,
			      session->event_cb_arg, &quit_now);
      if (quit_now)
        exts |= EXT_ETRN;
    }
#endif
  return !exts;
}

static int
cb_ehlo (smtp_session_t session, char *buf)
{
  const char *p;
  char token[32];

  if (!read_atom (skipblank (buf), &p, token, sizeof token))
    {
      /* expecting an atom - do nothing */
      return 0;
    }

  /* Since the session structure mostly just carries a bit for each of
     the extensions, there is no point #ifdefing out the extension
     keywords for omitted features.  Also extensions may be added
     in the future so it is not an error to have an unrecognised
     extension keyword. */

  if (strcasecmp (token, "ENHANCEDSTATUSCODES") == 0)	/* RFC 3463, RFC 2034 */
    session->extensions |= EXT_ENHANCEDSTATUSCODES;
  else if (strcasecmp (token, "PIPELINING") == 0)	/* RFC 2920 */
    session->extensions |= EXT_PIPELINING;
  else if (strcasecmp (token, "DSN") == 0)		/* RFC 3461 */
    session->extensions |= EXT_DSN;
  else if (strcasecmp (token, "AUTH") == 0)		/* RFC 4954 */
    {
      session->extensions |= EXT_AUTH;
      set_auth_mechanisms (session, p);
    }
#ifdef AUTH_ID_HACK
  else if (strncasecmp (token, "AUTH=", 5) == 0) /* non-standard syntax */
    {
      session->extensions |= EXT_AUTH;
      set_auth_mechanisms (session, token + 5);
      set_auth_mechanisms (session, p);
    }
#endif
  else if (strcasecmp (token, "STARTTLS") == 0)		/* RFC 3207 */
    session->extensions |= EXT_STARTTLS;
  else if (strcasecmp (token, "SIZE") == 0)		/* RFC 1870 */
    {
      session->extensions |= EXT_SIZE;
      session->size_limit = strtol (p, NULL, 10);
    }
  else if (strcasecmp (token, "CHUNKING") == 0)		/* RFC 3030 */
    session->extensions |= EXT_CHUNKING;
  else if (strcasecmp (token, "BINARYMIME") == 0)	/* RFC 3030 */
    session->extensions |= EXT_BINARYMIME;
  else if (strcasecmp (token, "8BITMIME") == 0)		/* RFC 6152 */
    session->extensions |= EXT_8BITMIME;
  else if (strcasecmp (token, "DELIVERBY") == 0)	/* RFC 2852 */
    {
      session->extensions |= EXT_DELIVERBY;
      session->min_by_time = strtol (p, NULL, 10);
    }
  else if (strcasecmp (token, "ETRN") == 0)		/* RFC 1985 */
    session->extensions |= EXT_ETRN;
  else if (strcasecmp (token, "XUSR") == 0)	/* sendmail (I feel ill) */
    session->extensions |= EXT_XUSR;
  else if (strcasecmp (token, "XEXCH50") == 0)	/* exchange (I feel worse) */
    session->extensions |= EXT_XEXCH50;
  return 1;
}


void
rsp_ehlo (siobuf_t conn, smtp_session_t session)
{
  int code;

  session->extensions = 0;
  destroy_auth_mechanisms (session);
  code = read_smtp_response (conn, session, &session->mta_status, cb_ehlo);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }

  if (code != 2)
    session->extensions = 0;

  if (code == 4)
    {
      /* 4xx failure code.  Something is temporarily wrong.  Fail the
         entire session and let the application retry later. */
      session->rsp_state = S_quit;
      session->try_fallback_server = 1;
      return;
    }
  else if (code == 5)
    {
      /* 5xx failure code.  Something is permanently wrong.  There are
         a number of codes indicating that HELO is worth a try since
         the server did not understand EHLO.  Otherwise fail the entire
         session.  The application must correct something and retry later. */
      if (session->mta_status.code == 500 || session->mta_status.code == 501
          || session->mta_status.code == 502 || session->mta_status.code == 504)
	session->rsp_state = S_helo;
      else
	session->rsp_state = S_quit;
      return;
    }
  else if (code != 2)
    {
      /* Response must be 2xx, 4xx or 5xx */
      set_error (SMTP_ERR_INVALID_RESPONSE_STATUS);
      session->rsp_state = S_quit;
      return;
    }

#ifdef USE_TLS
  /* Totally ignore the TLS stuff if it's already in use */
  if (!session->using_tls && session->starttls_enabled != Starttls_DISABLED)
    {
      if (select_starttls (session))
	{
	  session->rsp_state = S_starttls;
	  return;
	}
      if (session->starttls_enabled == Starttls_REQUIRED)
	{
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_EXTNA_STARTTLS,
				  session->event_cb_arg, NULL);
	  session->rsp_state = S_quit;
	  set_error (SMTP_ERR_EXTENSION_NOT_AVAILABLE);
	  return;
	}
    }
#endif
  /* If AUTH is enabled but no mechanisms can be selected, move on to the
     MAIL command since the MTA is required to accept mail for its own
     domain. */
  if ((session->extensions & EXT_AUTH) && select_auth_mechanism (session))
    {
      session->rsp_state = S_auth;
      return;
    }

  /* Report extensions *after* starting TLS or doing AUTH since either
     of these can restart the session with different SMTP extensions
     being offered by the server. */
  if (!report_extensions (session))
    {
      set_error (SMTP_ERR_EXTENSION_NOT_AVAILABLE);
      session->rsp_state = S_quit;
      return;
    }

#ifdef USE_ETRN
  session->rsp_state = check_etrn (session)
			  ? S_etrn : initial_transaction_state (session);
#else
  session->rsp_state = initial_transaction_state (session);
#endif
}

/* Select the correct initial state after reading the EHLO response or
   after DATA or RSET in the previous transaction.  */
int
initial_transaction_state (smtp_session_t session)
{
#ifdef USE_XUSR
  if (session->extensions & EXT_XUSR)
    return S_xusr;
#endif
  return S_mail;
}

/*****************************************************************************
 * HELO 
 *****************************************************************************/

/* HELO is absolutely *not* the preferred client greeting to the server.
   The parameter is the FQDN of the client host.  The server response is
   the greeting line.  No capabilities are reported so all extensions
   are turned off.

   Next state is Mail.
 */
void
cmd_helo (siobuf_t conn, smtp_session_t session)
{
  sio_printf (conn, "HELO %s\r\n", session->localhost);
  session->cmd_state = -1;
}

void
rsp_helo (siobuf_t conn, smtp_session_t session)
{
  int code;
#ifdef USE_TLS
  int notls;
#endif

  session->extensions = 0;
  destroy_auth_mechanisms (session);
  code = read_smtp_response (conn, session, &session->mta_status, NULL);
  if (code < 0)
    {
      session->try_fallback_server = 1;
      session->rsp_state = S_quit;
      return;
    }
  if (code != 2)
    {
      if (!(code == 4 || code == 5))
	set_error (SMTP_ERR_INVALID_RESPONSE_STATUS);
      session->try_fallback_server = 1;
      session->rsp_state = S_quit;
      return;
    }

#ifdef USE_TLS
  /* Care must be taken when checking for required TLS.  The client may
     have connected to a tunnel which offers the STARTTLS extension and
     the real server does not implement SMTP extensions.  Hence the
     check that TLS is actually in use before reporting that the
     extension is not available.  */
  notls = !session->using_tls && session->starttls_enabled == Starttls_REQUIRED;
  if (notls && session->event_cb != NULL)
    (*session->event_cb) (session, SMTP_EV_EXTNA_STARTTLS,
			  session->event_cb_arg, NULL);
#else
# define notls 0
#endif

  /* There are no extensions.  Make sure none were required. */
  if (!report_extensions (session) || notls)
    {
      set_error (SMTP_ERR_EXTENSION_NOT_AVAILABLE);
      session->rsp_state = S_quit;
      return;
    }

  /* Unlike EHLO, the only next state can be Mail, since there are
     no extensions to check for message acceptability or options to set
     before proceeding. */
  session->rsp_state = initial_transaction_state (session);
}

/*****************************************************************************
 * MAIL FROM: 
 *****************************************************************************/

/* MAIL FROM: is the first step in sending a message.  Select the first
   or a subsequent message from the session structure.  The message sender
   is taken from the message structure.

   Next state is always Rcpt, therefore this command need not be flushed.
 */
void
cmd_mail (siobuf_t conn, smtp_session_t session)
{
  const char *mailbox;
  smtp_message_t message;
  char xtext[256];

  /* Set a five minute timeout.  This stays in force until the DATA
     command. */
  sio_set_timeout (conn, session->envelope_timeout);

  message = session->current_message;
  mailbox = message->reverse_path_mailbox;
  sio_printf (conn, "MAIL FROM:<%s>", (mailbox != NULL) ? mailbox : "");

  /* SIZE: SIZE=message-size-estimate */
  if ((session->extensions & EXT_SIZE) && message->size_estimate > 0)
    sio_printf (conn, " SIZE=%lu", message->size_estimate);

  /* DSN: RET=FULL/HDRS  ENVID=xtext */
  if (session->extensions & EXT_DSN)
    {
      static const char *ret[] = { NULL, "FULL", "HDRS" };

      if (message->dsn_ret != Ret_NOTSET)
	sio_printf (conn, " RET=%s", ret[message->dsn_ret]);

      if (message->dsn_envid != NULL)
	sio_printf (conn, " ENVID=%s",
		    encode_xtext (xtext, sizeof xtext, message->dsn_envid));
    }

  /* 8BITMIME: BODY=7BIT/8BITMIME/BINARYMIME */
#ifdef USE_CHUNKING
  if ((session->extensions & (EXT_8BITMIME | EXT_BINARYMIME))
#else
  if ((session->extensions & EXT_8BITMIME)
#endif
      && message->e8bitmime != E8bitmime_NOTSET)
    {
      sio_write (conn, " BODY=", -1);
      if (message->e8bitmime == E8bitmime_8BITMIME)
	sio_write (conn, "8BITMIME", -1);
      else if (message->e8bitmime == E8bitmime_7BIT)
	sio_write (conn, "7BIT", -1);
#ifdef USE_CHUNKING
      else if (message->e8bitmime == E8bitmime_BINARYMIME)
	sio_write (conn, "BINARYMIME", -1);
#endif
    }

  if ((session->extensions & EXT_DELIVERBY) && message->by_mode != By_NOTSET)
    {
      static char mode[] = { 'N', 'R', };
      long by_time;

      by_time = message->by_time;
      /* If the by_time is greater than the server's min_by_time, ask the
	 application what to do.  If adjust is set > 0, the message's
	 deliver by time is adjusted to be acceptable to the server.
	 If not, the MAIL command will be failed by the server.  */
      if (session->min_by_time > 0 && by_time < session->min_by_time)
	{
	  int adjust = 0;

	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_DELIVERBY_EXPIRED,
				  session->event_cb_arg,
				  session->min_by_time - by_time, &adjust);
	  if (adjust > 0)
	    by_time = session->min_by_time + adjust;
	}
      sio_printf (conn, " BY=%ld%c%s", by_time,
      		  mode[message->by_mode], (message->by_trace) ? "T" : "");
    }

  sio_write (conn, "\r\n", 2);
  /* TODO: until code to prevent issuing of further RCPT commands and to
           discard RCPT responses cascading from an error response to
           MAIL is in place, flush the mail command even when pipelining. */
#if 0
  session->cmd_state = S_rcpt;
#else
  session->cmd_state = -1;
#endif
}

void
rsp_mail (siobuf_t conn, smtp_session_t session)
{
  int code;
  smtp_message_t message;

  message = session->current_message;
  code = read_smtp_response (conn, session,
  		             &message->reverse_path_status, NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }

  /* Notify the MAIL FROM: status */
  if (session->event_cb != NULL)
    (*session->event_cb) (session, SMTP_EV_MAILSTATUS, session->event_cb_arg,
  			  message->reverse_path_mailbox, message);
  if (code != 2)
    {
      if (next_message (session))
	session->rsp_state = initial_transaction_state (session);
      else
	session->rsp_state = S_quit;
    }
  else
    {
      message->valid_recipients = 0;
      message->failed_recipients = 0;
      session->rsp_state = S_rcpt;
    }
}

/*****************************************************************************
 * RCPT TO:
 *****************************************************************************/

/* Specify one message recipient.  This is taken from the recipient
   parameters.  Many parameters are possible depending on the extensions
   enabled.  For errors such as unknown recipient, or cannot relay,
   just mark the offending recipient as failing and continue normally.

   Next state is Rcpt, Data or Bdat.  This depends on the number of
   recipients for the message and the extensions enabled.  This selection
   can be made without waiting for the server response therefore this
   command need not be flushed.
 */
void
cmd_rcpt (siobuf_t conn, smtp_session_t session)
{
  static struct { enum notify_flags mask; const char *flag; } masks[] =
    {
      { Notify_SUCCESS, "SUCCESS", },
      { Notify_FAILURE, "FAILURE", },
      { Notify_DELAY,   "DELAY", },
    };
  smtp_recipient_t recipient;
  enum notify_flags notify;
  char xtext[256];
  int i;

  recipient = session->cmd_recipient;
  sio_printf (conn, "RCPT TO:<%s>", recipient->mailbox);

  if (session->extensions & EXT_DSN)
    {
      /* DSN: NOTIFY=NEVER/SUCCESS,FAILURE,DELAY */
      notify = recipient->dsn_notify;
      if (notify != Notify_NOTSET)
	{
	  sio_write (conn, " NOTIFY=", -1);
	  if (notify == Notify_NEVER)
	    sio_write (conn, "NEVER", -1);
	  else
	    for (i = 0; i < (int) (sizeof masks / sizeof masks[0]); i++)
	      if (notify & masks[i].mask)
		{
		  notify &= ~masks[i].mask;
		  sio_write (conn, masks[i].flag, -1);
		  if (notify != 0)
		    sio_write (conn, ",", 1);
		}
	}

      /* DSN: ORCPT=type;address */
      if (recipient->dsn_orcpt != NULL)
	sio_printf (conn, " ORCPT=%s;%s", recipient->dsn_addrtype,
		    encode_xtext (xtext, sizeof xtext, recipient->dsn_orcpt));
    }
  sio_write (conn, "\r\n", 2);

  session->cmd_recipient = next_recipient (session->cmd_recipient);
  if (session->cmd_recipient != NULL)
    session->cmd_state = S_rcpt;
  else if (session->require_all_recipients)
    /* can't pipeline the DATA command when require_all_recpients is set. */
    session->cmd_state = -1;
  else
#ifdef USE_CHUNKING
    session->cmd_state = (session->extensions & EXT_CHUNKING) ? S_bdat : S_data;
#else
    session->cmd_state = S_data;
#endif
}

void
rsp_rcpt (siobuf_t conn, smtp_session_t session)
{
  int code;

  code = read_smtp_response (conn, session,
  			     &session->rsp_recipient->status, NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }

  if (code == 2)
    session->current_message->valid_recipients += 1;
  else
    session->current_message->failed_recipients += 1;

  /* MTA will never accept this recipient.  Make sure it isn't used
     again. */
  if (code == 5)
    session->rsp_recipient->complete = 1;

  /* Notify the RCPT TO: status */
  if (session->event_cb != NULL)
    (*session->event_cb) (session, SMTP_EV_RCPTSTATUS, session->event_cb_arg,
  			  session->rsp_recipient->mailbox,
  			  session->rsp_recipient);

  session->rsp_recipient = next_recipient (session->rsp_recipient);
  if (session->rsp_recipient != NULL)
    session->rsp_state = S_rcpt;
  else if (session->require_all_recipients
           && session->current_message->failed_recipients > 0)
    {
      reset_status (&session->current_message->message_status);
      session->rsp_state = next_message (session) ? S_rset : S_quit;
    }
  else
#ifdef USE_CHUNKING
    session->rsp_state = (session->extensions & EXT_CHUNKING) ? S_bdat : S_data;
#else
    session->rsp_state = S_data;
#endif
}

/*****************************************************************************
 * DATA
 *****************************************************************************/

void
cmd_data (siobuf_t conn, smtp_session_t session)
{
  sio_set_timeout (conn, session->data_timeout);
  sio_write (conn, "DATA\r\n", -1);
  session->cmd_state = -1;
}

void
rsp_data (siobuf_t conn, smtp_session_t session)
{
  int code;
  smtp_message_t message;

  message = session->current_message;
  code = read_smtp_response (conn, session, &message->message_status, NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }
  if (code == 4 || code == 5)
    {
      /* The server will not accept this message.
       */

      /* N.B.  This is a bit tricky.  RFC 821 isn't clear and I can't
	 find any information in RFC 2821 either but what state is the
	 SMTP server supposed to be in when the DATA command fails a)
	 when the 354 response is expected and b) after the message is
	 copied to the server.

	 Play safe and issue the RSET command.  */
      if (next_message (session))
	session->rsp_state = S_rset;
      else
	session->rsp_state = S_quit;
    }
  else if (code != 3)
    {
      set_error (SMTP_ERR_INVALID_RESPONSE_STATUS);
      session->rsp_state = S_quit;
    }
  else
    session->rsp_state = S_data2;

  /* Notify end of message here if not transferring anything */
  if (code != 3 && session->event_cb != NULL)
    (*session->event_cb) (session, SMTP_EV_MESSAGESENT,
    			  session->event_cb_arg, message);
}

/* Read the message from the application using the callback.
   Break into lines and copy to the server. */
void
cmd_data2 (siobuf_t conn, smtp_session_t session)
{
  const char *line, *header, *pline, *p;
  int c, len;

  /* RFC 2920 - some servers may return a 354 response to DATA even
     if there are no valid recipients.  If this happens just send a
     line containing .\r\n to terminate the command.  It will then
     fail as expected. */
  if (session->current_message->valid_recipients == 0)
    {
      sio_write (conn, ".\r\n", 3);
      session->cmd_state = -1;
      return;
    }

  sio_set_timeout (conn, session->transfer_timeout);

  /* Arrange to read the current message from the application. */
  msg_source_set_cb (session->msg_source,
		     session->current_message->cb,
		     session->current_message->cb_arg);

  /* Arrange *not* to have the message contents monitored.  This is
     purely to avoid overwhelming the application with data. */
  sio_set_monitorcb (conn, NULL, NULL);

  /* Make sure we read the message from the beginning and get
     the header processing right.  */
  msg_rewind (session->msg_source);
  reset_header_table (session->current_message);

  /* Read and process header lines from the application.
     This step in processing
     i)   removes headers provided by the application that should not be
          present in the message, e.g. Return-Path: which is added by
          an MTA during delivery but should not be present in a message
          being submitted or which is in transit.
     ii)  copies certain headers verbatim, e.g. MIME headers.
     iii) alters the content of certain headers.  This will happen
          according to library options set up by the application.
   */
  errno = 0;
  while ((line = msg_gets (session->msg_source, &len, 0)) != NULL)
    {
      /* Header processing stops at a line containing only CRLF */
      if (len == 2 && line[0] == '\r' && line[1] == '\n')
	break;

      /* Check for continuation lines indicated by a blank or tab
         at the start of the next line. */
      while ((c = msg_nextc (session->msg_source)) != -1)
	{
	  if (c != ' ' && c != '\t')
	    break;
	  line = msg_gets (session->msg_source, &len, 1);
	  if (line == NULL)
	    goto break_2;
	}

      /* Line points to one or more lines of text forming an RFC 5322
	 header. */

      /* Header processing.  This function takes the "raw" header from
         the application and returns a header which is to be written
         to the remote MTA.   If header is NULL this header has been
         deleted by the library.  If header == line it is passed
         unchanged otherwise, header must be freed after use. */
      header = process_header (session->current_message, line, &len);
      if (header != NULL && len > 0)
	{
	  /* Notify byte count to the application. */
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_MESSAGEDATA,
				  session->event_cb_arg,
				  session->current_message, len);

	  /* During data transfer, if we are monitoring the message
	     headers, call the monitor callback directly, once per header.
	     We don't bother with monitoring the dot stuffing.	Also set
	     the value of the writing parameter to 2 so that the app can
	     distinguish headers from data written in the sio_ package.  */
	  if (session->monitor_cb && session->monitor_cb_headers)
	    (*session->monitor_cb) (header, len, SMTP_CB_HEADERS,
	    			    session->monitor_cb_arg);

	  /* Write the header using dot stuffing.  N.B. because of
	     dot stuffing, it is necessary to find the line breaks
	     during the copy. */
	  for (pline = header; pline < header + len; pline = p)
	    {
	      p = memchr (pline, '\n', header + len - pline);
	      if (p == NULL)
	        {
		  set_errno (ERANGE);
		  session->cmd_state = session->rsp_state = -1;
		  return;
	        }
	      if (pline[0] == '.')
		sio_write (conn, ".", 1);
	      sio_write (conn, pline, ++p - pline);
	    }
	}
      errno = 0;
    }
break_2:
  if (errno != 0)
    {
      /* An error occurred during processing.  The only thing that can
         be done is to drop the connection to the server since SMTP has
         no way to recover gracefully from client errors while transferring
         the message. */
      set_errno (errno);
      session->cmd_state = session->rsp_state = -1;
      return;
    }

  /* Now send missing headers.  This completes the processing started
     above.  If the application did not supply certain headers that
     should be present, the library will supply them here, e.g. Date:,
     Message-Id: or To:/Cc:/Bcc: headers.  In the most extreme case the
     application might just send a CRLF followed by the message body.
     Libesmtp will then provide all the necessary headers. */
  while ((header = missing_header (session->current_message, &len)) != NULL)
    if (len > 0)
      {
	/* Notify byte count to the application. */
	if (session->event_cb != NULL)
	  (*session->event_cb) (session, SMTP_EV_MESSAGEDATA,
				session->event_cb_arg,
				session->current_message, len);

	if (session->monitor_cb && session->monitor_cb_headers)
	  (*session->monitor_cb) (header, len, SMTP_CB_HEADERS,
				  session->monitor_cb_arg);
	for (pline = header; pline < header + len; pline = p)
	  {
	    p = memchr (pline, '\n', header + len - pline);
	    if (p == NULL)
	      {
		set_errno (ERANGE);
		session->cmd_state = session->rsp_state = -1;
		return;
	      }
	    if (pline[0] == '.')
	      sio_write (conn, ".", 1);
	    sio_write (conn, pline, ++p - pline);
	  }
      }

  /* ... and finally terminate the message headers */
  sio_write (conn, "\r\n", 2);

  /* Read message body lines from the application and write them
     to the remote MTA using dot stuffing. */
  errno = 0;
  while ((line = msg_gets (session->msg_source, &len, 0)) != NULL)
    {
      /* Notify byte count to the application. */
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_MESSAGEDATA,
	                      session->event_cb_arg,
	                      session->current_message, len);

      if (line[0] == '.')
	sio_write (conn, ".", 1);
      sio_write (conn, line, len);
      errno = 0;
    }
  if (errno != 0)
    {
      set_errno (errno);
      session->cmd_state = session->rsp_state = -1;
      return;
    }

  /* Terminate the DATA command.  Explicitly flush the buffer here.
     This would have happened in the protocol loop anyway but doing it
     here makes the output of strace more intuitive. */
  sio_write (conn, ".\r\n", 3);
  sio_flush (conn);

  sio_set_timeout (conn, session->data2_timeout);
  session->cmd_state = -1;
}

void
rsp_data2 (siobuf_t conn, smtp_session_t session)
{
  int code;
  smtp_recipient_t recipient;

  /* Reinstate the protocol monitor. */
  if (session->monitor_cb != NULL)
    sio_set_monitorcb (conn, session->monitor_cb, session->monitor_cb_arg);

  code = read_smtp_response (conn, session,
			     &session->current_message->message_status,
			     NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }

  if (code == 2)
    {
      /* Mark all the recipients complete for which the MTA has accepted
         responsibility for delivery.  */
      for (recipient = session->current_message->recipients;
	   recipient != NULL;
	   recipient = recipient->next)
	if (!recipient->complete
	    && recipient->status.code >= 200 && recipient->status.code <= 299)
	  recipient->complete = 1;
    }
  else if (code == 5)
    {
      /* Mark all the recipients complete.  This message cannot be
         accepted for any recipients.  */
      for (recipient = session->current_message->recipients;
	   recipient != NULL;
	   recipient = recipient->next)
	recipient->complete = 1;
    }


  if (session->event_cb != NULL)
    (*session->event_cb) (session, SMTP_EV_MESSAGESENT,
                          session->event_cb_arg, session->current_message);

  if (next_message (session))
    session->rsp_state = (code == 2) ? initial_transaction_state (session)
                                     : S_rset;
  else
    session->rsp_state = S_quit;
}

/*****************************************************************************
 * RSET
 *****************************************************************************/

void
cmd_rset (siobuf_t conn, smtp_session_t session)
{
  sio_write (conn, "RSET\r\n", 6);
  if (session->current_message != NULL)
    session->cmd_state = initial_transaction_state (session);
  else
    session->cmd_state = S_quit;
}

void
rsp_rset (siobuf_t conn, smtp_session_t session)
{
  struct smtp_status status;

  /* The RSET command should always succeed, since this client never
     attempts to sent trailing whitespace or parameters to the command */
  memset (&status, 0, sizeof status);
  read_smtp_response (conn, session, &status, NULL);
  reset_status (&status);
  if (session->current_message != NULL)
    session->rsp_state = initial_transaction_state (session);
  else
    session->rsp_state = S_quit;
}

/*****************************************************************************
 * QUIT
 *****************************************************************************/

void
cmd_quit (siobuf_t conn, smtp_session_t session)
{
  sio_write (conn, "QUIT\r\n", 6);
  session->cmd_state = -1;
}

void
rsp_quit (siobuf_t conn, smtp_session_t session)
{
  struct smtp_status status;

  /* The QUIT command should always succeed.
   */
  memset (&status, 0, sizeof status);
  read_smtp_response (conn, session, &status, NULL);
  reset_status (&status);
  session->rsp_state = -1;
}

/*****************************************************************************
 * Other crud.
 *****************************************************************************/

#ifdef USE_XUSR
void
cmd_xusr (siobuf_t conn, smtp_session_t session)
{
  sio_write (conn, "XUSR\r\n", 6);
  session->cmd_state = -1;
}

void
rsp_xusr (siobuf_t conn, smtp_session_t session)
{
  struct smtp_status status;

  /* The XUSR command should always succeed?  */
  memset (&status, 0, sizeof status);
  read_smtp_response (conn, session, &status, NULL);
  reset_status (&status);
  session->rsp_state = S_mail;
}
#endif
