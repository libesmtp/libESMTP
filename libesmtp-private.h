#ifndef _libesmtp_private_h
#define _libesmtp_private_h
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

#include <stddef.h>		/* for size_t */

#ifdef USE_TLS
#include <openssl/ssl.h>
#endif
#include "libesmtp.h" /* The library needs the public declarations too! */
#include "message-source.h"
#include "concatenate.h"
#include "auth-client.h"

/* SMTP Extensions */

#define _BIT(n)		(1 << (n))

#define EXT_ENHANCEDSTATUSCODES	_BIT(0)	/* RFC 3463, RFC 2034 */
#define EXT_PIPELINING		_BIT(1)	/* RFC 2920 */
#define EXT_DSN			_BIT(2)	/* RFC 1891 */
#define EXT_AUTH		_BIT(3)	/* RFC 4954 AUTH using SASL */
#define EXT_STARTTLS		_BIT(4)	/* RFC 3207 */
#define EXT_SIZE		_BIT(5)	/* RFC 1870 */
#define EXT_CHUNKING		_BIT(6)	/* RFC 3030 */
#define EXT_BINARYMIME		_BIT(7)	/* RFC 3030 */
#define EXT_8BITMIME		_BIT(8)	/* RFC 6152 */
#define EXT_DELIVERBY		_BIT(9)	/* RFC 2852 */
#define EXT_ETRN		_BIT(10)/* RFC 1985 */
#define EXT_XUSR		_BIT(11)/* sendmail */
#define EXT_XEXCH50		_BIT(12)/* exchange */

struct smtp_session
  {
  /* Local info */
    char *localhost;			/* Domain name of localhost */

  /* MTA */
    char *host;				/* Host domain name of SMTP server */
    char *canon;			/* Canonic host name of SMTP server */
    const char *port;			/* Port number - default to 587 */

  /* Application data */
    void *application_data;		/* Pointer to data maintained by app */
    void (*release) (void *);		/* function to free/unref data */

  /* Messages */
    struct smtp_message *messages;	/* list of messages to submit */
    struct smtp_message *end_messages;

  /* Protocol events */
    smtp_eventcb_t event_cb;		/* Protocol event callback */
    void *event_cb_arg;			/* Argument for above */

  /* Protocol monitor */
    smtp_monitorcb_t monitor_cb;	/* Protocol monitor callback */
    void *monitor_cb_arg;		/* Argument for above */
    int monitor_cb_headers;		/* Monitor views message headers */

  /* Variables used by the protocol state engine */
    int cmd_state, rsp_state;
    struct smtp_message *current_message;
    struct smtp_recipient *cmd_recipient;
    struct smtp_recipient *rsp_recipient;
    msg_source_t msg_source;

  /* SMTP timeouts */
    long greeting_timeout;		/* default 5 minutes */
    long envelope_timeout;		/* default 5 minutes */
    long data_timeout;			/* default 2 minutes */
    long transfer_timeout;		/* default 3 minutes */
    long data2_timeout;			/* default 10 minutes */

  /* Status */
    smtp_status_t mta_status;		/* Status from MTA greeting */

  /* Protocol extensions */
    unsigned long extensions;
    unsigned long required_extensions;
    unsigned long size_limit;		/* RFC 1870 */
    long min_by_time;			/* RFC 2852 */

  /* Interface to RFC 4954 AUTH and SASL */
    auth_context_t auth_context;
    struct mechanism *auth_mechanisms;
    struct mechanism *current_mechanism;

#ifdef USE_ETRN
  /* Interface to RFC 1985 ETRN extension */
    struct smtp_etrn_node *etrn_nodes;
    struct smtp_etrn_node *end_etrn_nodes;
    struct smtp_etrn_node *cmd_etrn_node;
    struct smtp_etrn_node *rsp_etrn_node;
#endif

#ifdef USE_TLS
  /* Interface to RFC 3207 STARTTLS extension */
    enum starttls_option starttls_enabled;
    SSL_CTX *starttls_ctx;
#endif

#ifdef USE_CHUNKING
    int bdat_pipelined;
#endif

  /* Miscellaneous options and flags */
    unsigned int try_fallback_server : 1;
    unsigned int require_all_recipients : 1;
    unsigned int authenticated : 1;
#ifdef USE_CHUNKING
    unsigned int bdat_abort_pipeline : 1;
    unsigned int bdat_last_issued : 1;
#endif
#ifdef USE_TLS
    unsigned int using_tls : 1;
#endif
  };

struct smtp_message
  {
    struct smtp_message *next;
    struct smtp_session *session;	/* Back reference */

  /* Application data */
    void *application_data;		/* Pointer to data maintained by app */
    void (*release) (void *);		/* function to free/unref data */

  /* Reverse Path */
    char *reverse_path_mailbox;		/* Reverse path */

  /* Status */
    smtp_status_t reverse_path_status;	/* Reverse path status from MAIL */
    smtp_status_t message_status;	/* Message status from DATA/BDAT */

  /* Recipients */
    struct smtp_recipient *recipients;	/* List of recipients */
    struct smtp_recipient *end_recipients;
    int valid_recipients;		/* Valid recipients in this session */
    int failed_recipients;		/* Failed recipients in this session */

  /* Headers */
    struct rfc2822_header *headers;	/* List of headers to add to message */
    struct rfc2822_header *end_headers;
    struct rfc2822_header *current_header;
    struct h_node **hdr_action;		/* Hash table for header action */
    struct catbuf hdr_buffer;		/* Buffer for printing headers */

  /* Message */
    smtp_messagecb_t cb;		/* Transfer message from app. */
    void *cb_arg;			/* Argument for above */

  /* DSN  (RFC 3461) */
    char *dsn_envid;			/* envelope identifier */
    enum ret_flags dsn_ret;		/* return headers or entire message */

  /* SIZE  (RFC 1870) */
    unsigned long size_estimate;

  /* DELIVERBY  (RFC 2852 ) */
    long by_time;
    enum by_mode by_mode;
    int by_trace;

  /* 8BITMIME  (RFC 6152) */
    enum e8bitmime_body e8bitmime;
  };

struct smtp_recipient
  {
    struct smtp_recipient *next;
    struct smtp_message *message;	/* Back reference */

  /* Application data */
    void *application_data;		/* Pointer to data maintained by app */
    void (*release) (void *);		/* function to free/unref data */

  /* Recipient Info */
    char *mailbox;			/* Envelope address */
    smtp_status_t status;		/* Recipient status from RCPT */
    unsigned complete : 1;		/* Sent OK or permanent failure */
    /* more per recipient stuff */

  /* DSN  - (RFC 3461) */
    char *dsn_addrtype;			/* original recipient address type */
    char *dsn_orcpt;			/* original recipient */
    enum notify_flags dsn_notify;	/* notification options */
  };

#define APPEND_LIST(start,end,item)	do {				\
					  if ((start) == NULL)		\
					    (start) = (item);		\
					  else				\
					    (end)->next = (item);	\
					  (end) = (item);		\
					  (item)->next = NULL;		\
					} while (0)

/* RFC 5321 minimum timeouts */

#define GREETING_DEFAULT	( 5 * 60l * 1000l)
#define ENVELOPE_DEFAULT	( 5 * 60l * 1000l)
#define DATA_DEFAULT		( 2 * 60l * 1000l)
#define TRANSFER_DEFAULT	( 3 * 60l * 1000l)
#define DATA2_DEFAULT		(10 * 60l * 1000l)

/* protocol.c */

int initial_transaction_state (smtp_session_t session);
int next_message (smtp_session_t session);

/* errors.c */

void set_error (int code);
void set_errno(int code);
void set_herror (int code);
int do_session (smtp_session_t session);
void reset_status (struct smtp_status *status);

/* smtp-auth.c */

void set_auth_mechanisms (smtp_session_t session, const char *mechanisms);
void destroy_auth_mechanisms (smtp_session_t session);
int select_auth_mechanism (smtp_session_t session);

#ifdef USE_TLS
/* smtp-tls.c */

int select_starttls (smtp_session_t session);
void destroy_starttls_context (smtp_session_t session);
#endif

#ifdef USE_ETRN
/* smtp-etrn.c */

int check_etrn (smtp_session_t session);
void destroy_etrn_nodes (smtp_session_t session);

#endif

#endif
