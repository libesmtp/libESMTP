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

/* Support for the SMTP BDAT verb (CHUNKING).  The code for processing
   headers is duplicated from the DATA verb's code.  Make sure to keep
   bug fixes in sync with that code.  Message body and response
   processing is specific to the BDAT code and bears no resemblance to
   that of the DATA command.  */

#include <config.h>

#ifdef USE_CHUNKING

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <missing.h> /* declarations for missing library functions */

#include "libesmtp-private.h"
#include "message-source.h"
#include "siobuf.h"
#include "concatenate.h"
#include "headers.h"
#include "protocol.h"

/* Read the message from the application using the callback.
   Break into chunks and copy to the server. */
void
cmd_bdat (siobuf_t conn, smtp_session_t session)
{
  const char *line, *header, *chunk;
  int c, len;
  struct catbuf headers;

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

  /* Initialise a buffer for the message headers. */
  cat_init (&headers, 1024);

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
      if (header != NULL)
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

	  /* Accumulate the header line into a buffer */
	  concatenate (&headers, header, len);
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
    {
      /* Notify byte count to the application. */
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_MESSAGEDATA,
	                      session->event_cb_arg,
	                      session->current_message, len);

      if (session->monitor_cb && session->monitor_cb_headers)
	(*session->monitor_cb) (header, len, SMTP_CB_HEADERS,
				session->monitor_cb_arg);
      /* Accumulate the header line into a buffer */
      concatenate (&headers, header, len);
    }

  /* Terminate headers */
  concatenate (&headers, "\r\n", 2);

  /* ``headers'' now contains the message headers.  Transfer them in a
     BDAT command and move to the next state. */
  session->bdat_abort_pipeline = 0;
  session->bdat_last_issued = 0;
  session->bdat_pipelined = 1;
  chunk = cat_buffer (&headers, &len);
  sio_printf (conn, "BDAT %d\r\n", len);
  sio_write (conn, chunk, len);
  cat_free (&headers);
  session->cmd_state = S_bdat2;
}

void
rsp_bdat (siobuf_t conn, smtp_session_t session)
{
  rsp_bdat2 (conn, session);
}

void
cmd_bdat2 (siobuf_t conn, smtp_session_t session)
{
  const char *chunk;
  int len;

  /* N.B. the BDAT chunk size is set by the amount of buffering
          provided by the application callback.  An application is not
          advised to read a message line by line in the callback.
          Instead it should buffer the message by a "reasonable" amount,
          say, 2Kb.  */
  errno = 0;
  chunk = msg_getb (session->msg_source, &len);
  if (chunk != NULL)
    {
      /* Notify byte count to the application. */
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_MESSAGEDATA,
	                      session->event_cb_arg,
	                      session->current_message, len);
      sio_printf (conn, "BDAT %d\r\n", len);
      sio_write (conn, chunk, len);

      /* BDAT commands may be pipelined.  Check if a a previous BDAT has
         failed and stop pipelining if necessary.  */
      session->cmd_state = session->bdat_abort_pipeline ? -1 : S_bdat2;
    }
  else
    {
      /* Workaround - M$ Exchange is broken wrt RFC 3030 */
      if (session->extensions & EXT_XEXCH50)
	sio_write (conn, "BDAT 2 LAST\r\n\r\n", -1);
      else
	sio_write (conn, "BDAT 0 LAST\r\n", -1);
      sio_set_timeout (conn, session->data2_timeout);
      session->bdat_last_issued = 1;
      session->cmd_state = -1;
    }
  session->bdat_pipelined += 1;
  if (errno != 0)
    {
      set_errno (errno);
      session->cmd_state = session->rsp_state = -1;
    }
}

void
rsp_bdat2 (siobuf_t conn, smtp_session_t session)
{
  int code;
  smtp_message_t message;
  smtp_recipient_t recipient;

  message = session->current_message;
  code = read_smtp_response (conn, session, &message->message_status, NULL);

  session->bdat_pipelined -= 1;
  if (code == 2)
    {
      if (session->bdat_pipelined > 0 || !session->bdat_last_issued)
	session->rsp_state = S_bdat2;
      else
        {
	  /* Mark all the recipients complete for which the MTA has accepted
	     responsibility for delivery.  */
	  for (recipient = session->current_message->recipients;
	       recipient != NULL;
	       recipient = recipient->next)
	    if (!recipient->complete
		&& recipient->status.code >= 200
		&& recipient->status.code <= 299)
	      recipient->complete = 1;

	  /* Notify `message sent' */
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_MESSAGESENT,
				  session->event_cb_arg,
				  session->current_message);

	  if (next_message (session))
	    session->rsp_state = initial_transaction_state (session);
	  else
	    session->rsp_state = S_quit;
        }
    }
  else
    {
      /* Stop further BDAT commands from being issued. */
      session->bdat_abort_pipeline = 1;

      if (session->bdat_pipelined > 0)
	session->rsp_state = S_bdat2;
      else
	{
	  /* Mark all the recipients complete.  This message cannot be
	     accepted for any recipients.  */
	  if (code == 5)
	    for (recipient = session->current_message->recipients;
		 recipient != NULL;
		 recipient = recipient->next)
	      recipient->complete = 1;

	  /* Notify `message sent' */
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_MESSAGESENT,
				  session->event_cb_arg,
				  session->current_message);

	  /* RFC 3030 is explicit that when the BDAT command fails
	     the server state is indeterminate and that a RSET
	     command must be issued.  If there are no more messages or
	     an unexpected status is received, just QUIT. */
	  if (!(code == 4 || code == 5))
	    {
	      set_error (SMTP_ERR_INVALID_RESPONSE_STATUS);
	      session->rsp_state = S_quit;
	    }
	  else if (next_message (session))
	    session->rsp_state = S_rset;
	  else
	    session->rsp_state = S_quit;
	}
    }
}
#endif
