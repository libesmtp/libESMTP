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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#ifdef HAVE_GETTIMEOFDAY
# include <sys/time.h>
#endif
#include <errno.h>

#include <missing.h>

#include "libesmtp-private.h"
#include "headers.h"
#include "htable.h"
#include "rfc2822date.h"
#include "api.h"
#include "attribute.h"

struct rfc2822_header
  {
    struct rfc2822_header *next;
    struct header_info *info;		/* Info for setting and printing */
    char *header;			/* Header name */
    void *value;			/* Header value */
  };

typedef int (*hdrset_t) (struct rfc2822_header *, va_list);
typedef void (*hdrprint_t) (smtp_message_t, struct rfc2822_header *);
typedef void (*hdrdestroy_t) (struct rfc2822_header *);

struct header_actions
  {
    const char *name;		/* Header for which the action is specified */
    unsigned int flags;		/* Header flags */
    hdrset_t set;		/* Function to set header value from API */
    hdrprint_t print;		/* Function to print the header value */
    hdrdestroy_t destroy;	/* Function to destroy the header value */
  };

struct header_info
  {
    const struct header_actions *action;
    struct rfc2822_header *hdr;	/* Pointer to most recently defined header */
    unsigned int seen : 1;	/* Header has been seen in the message */
    unsigned int override : 1;	/* LibESMTP is overriding the message */
    unsigned int prohibit : 1;	/* Header may not appear in the message */
  };

#define NELT(x)		((int) (sizeof x / sizeof x[0]))

#define OPTIONAL	0
#define SHOULD		1
#define REQUIRE		2
#define PROHIBIT	4
#define PRESERVE	8
#define LISTVALUE	16
#define MULTIPLE	32

static struct rfc2822_header *create_header (smtp_message_t message,
					    const char *header,
					    struct header_info *info);
void destroy_string (struct rfc2822_header *header);
void destroy_mbox_list (struct rfc2822_header *header);
struct header_info *find_header (smtp_message_t message,
				 const char *name, int len);
struct header_info *insert_header (smtp_message_t message, const char *name);

/* RFC 2822 headers processing */

/****************************************************************************
 * Functions for setting and printing header values
 ****************************************************************************/

static int
set_string (struct rfc2822_header *header, va_list alist)
{
  const char *value;

  assert (header != NULL);

  if (header->value != NULL)		/* Already set */
    return 0;

  value = va_arg (alist, const char *);
  if (value == NULL)
    return 0;
  header->value = strdup (value);
  return header->value != NULL;
}

static int
set_string_null (struct rfc2822_header *header, va_list alist)
{
  const char *value;

  assert (header != NULL);

  if (header->value != NULL)		/* Already set */
    return 0;

  value = va_arg (alist, const char *);
  if (value == NULL)
    return 1;
  header->value = strdup (value);
  return header->value != NULL;
}

/* Print header-name ": " header-value "\r\n" */
static void
print_string (smtp_message_t message, struct rfc2822_header *header)
{
  assert (message != NULL && header != NULL);

  /* TODO: implement line folding at white spaces */
  vconcatenate (&message->hdr_buffer, header->header, ": ",
		(header->value != NULL) ? header->value : "", "\r\n", NULL);
}

void
destroy_string (struct rfc2822_header *header)
{
  assert (header != NULL);

  if (header->value != NULL)
    free (header->value);
}

/* Print header-name ": <" message-id ">\r\n" */
static void
print_message_id (smtp_message_t message, struct rfc2822_header *header)
{
  const char *message_id;
  char buf[64];
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
#endif

  assert (message != NULL && header != NULL);

  message_id = header->value;
  if (message_id == NULL)
    {
#ifdef HAVE_GETTIMEOFDAY
      if (gettimeofday (&tv, NULL) != -1) /* This shouldn't fail ... */
	snprintf (buf, sizeof buf, "%ld.%ld.%d@%s", tv.tv_sec, tv.tv_usec,
		  getpid (), message->session->localhost);
      else /* ... but if it does fall back to using time() */
#endif
      snprintf (buf, sizeof buf, "%ld.%d@%s", time (NULL),
		getpid (), message->session->localhost);
      message_id = buf;
    }
  /* TODO: implement line folding at white spaces */
  vconcatenate (&message->hdr_buffer,
		header->header, ": <", message_id, ">\r\n",
		NULL);
}

/****/

static int
set_date (struct rfc2822_header *header, va_list alist)
{
  const time_t *value;

  assert (header != NULL);

  if ((time_t) header->value != (time_t) 0)		/* Already set */
    return 0;

  value = va_arg (alist, const time_t *);
  header->value = (void *) *value;
  return 1;
}

/* Print header-name ": " formatted-date "\r\n" */
static void
print_date (smtp_message_t message, struct rfc2822_header *header)
{
  char buf[64];
  time_t when;

  assert (message != NULL && header != NULL);

  when = (time_t) header->value;
  if (when == (time_t) 0)
    time (&when);
  vconcatenate (&message->hdr_buffer, header->header, ": ",
		rfc2822date (buf, sizeof buf, &when), "\r\n", NULL);
}

/****/

struct mbox
  {
    struct mbox *next;
    char *mailbox;
    char *phrase;
  };

void
destroy_mbox_list (struct rfc2822_header *header)
{
  struct mbox *mbox, *next;

  assert (header != NULL);

  mbox = header->value;
  while (mbox != NULL)
    {
      next = mbox->next;
      if (mbox->phrase != NULL)
	free ((void *) mbox->phrase);
      if (mbox->mailbox != NULL)
	free ((void *) mbox->mailbox);
      free (mbox);
      mbox = next;
    }
}

static int
set_from (struct rfc2822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

  assert (header != NULL);

  phrase = va_arg (alist, const char *);
  mailbox = va_arg (alist, const char *);

  /* Allow this to succeed as a special case.  Effectively requesting
     default action in print_from().   Fails if explicit values have
     already been set.	*/
  if (phrase == NULL && mailbox == NULL)
    return header->value == NULL;

  mbox = malloc (sizeof (struct mbox));
  if (mbox == NULL)
    return 0;
  mbox->phrase = (phrase != NULL) ? strdup (phrase) : NULL;
  mbox->mailbox = strdup (mailbox);

  mbox->next = header->value;
  header->value = mbox;
  return 1;
}

/* Print header-name ": " mailbox "\r\n"
      or header-name ": \"" phrase "\" <" mailbox ">\r\n" */
static void
print_from (smtp_message_t message, struct rfc2822_header *header)
{
  struct mbox *mbox;
  const char *mailbox;

  assert (message != NULL && header != NULL);

  vconcatenate (&message->hdr_buffer, header->header, ": ", NULL);
  /* TODO: implement line folding at white spaces */
  if (header->value == NULL)
    {
      mailbox = message->reverse_path_mailbox;
      vconcatenate (&message->hdr_buffer,
		    (mailbox != NULL && *mailbox != '\0') ? mailbox : "<>",
		    "\r\n", NULL);
    }
  else
    for (mbox = header->value; mbox != NULL; mbox = mbox->next)
      {
	mailbox = mbox->mailbox;
	if (mbox->phrase == NULL)
	  vconcatenate (&message->hdr_buffer,
			(mailbox != NULL && *mailbox != '\0') ? mailbox : "<>",
			NULL);
	else
	  vconcatenate (&message->hdr_buffer, "\"", mbox->phrase, "\""
			" <", (mailbox != NULL) ? mailbox : "", ">", NULL);
	vconcatenate (&message->hdr_buffer,
		      (mbox->next != NULL) ? ",\r\n    " : "\r\n", NULL);
      }
}

/* Same arguments and syntax as from: except that only one value is
   allowed.  */
static int
set_sender (struct rfc2822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

  assert (header != NULL);

  if (header->value != NULL)
    return 0;

  phrase = va_arg (alist, const char *);
  mailbox = va_arg (alist, const char *);
  if (phrase == NULL && mailbox == NULL)
    return 0;

  mbox = malloc (sizeof (struct mbox));
  if (mbox == NULL)
    return 0;
  mbox->phrase = (phrase != NULL) ? strdup (phrase) : NULL;
  mbox->mailbox = strdup (mailbox);
  mbox->next = NULL;

  header->value = mbox;
  return 1;
}

/* TODO: do nothing if the mailbox is NULL.  Check this doesn't fool
	 the protocol engine into thinking it has seen end of file. */
/* Print header-name ": " mailbox "\r\n"
      or header-name ": \"" phrase "\" <" mailbox ">\r\n"
 */
static void
print_sender (smtp_message_t message, struct rfc2822_header *header)
{
  struct mbox *mbox;
  const char *mailbox;

  assert (message != NULL && header != NULL);

  vconcatenate (&message->hdr_buffer, header->header, ": ", NULL);
  mbox = header->value;
  mailbox = mbox->mailbox;
  if (mbox->phrase == NULL)
    vconcatenate (&message->hdr_buffer,
		  (mailbox != NULL && *mailbox != '\0') ? mailbox : "<>",
		  "\r\n", NULL);
  else
    vconcatenate (&message->hdr_buffer, "\"", mbox->phrase, "\""
		  " <", (mailbox != NULL) ? mailbox : "", ">\r\n", NULL);
}

static int
set_to (struct rfc2822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

  assert (header != NULL);

  phrase = va_arg (alist, const char *);
  mailbox = va_arg (alist, const char *);
  if (phrase == NULL && mailbox == NULL)
    mbox = NULL;
  else
    {
      mbox = malloc (sizeof (struct mbox));
      if (mbox == NULL)
	return 0;
      mbox->phrase = (phrase != NULL) ? strdup (phrase) : NULL;
      mbox->mailbox = strdup (mailbox);

      mbox->next = header->value;
    }
  header->value = mbox;
  return 1;
}

static int
set_cc (struct rfc2822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

  assert (header != NULL);

  phrase = va_arg (alist, const char *);
  mailbox = va_arg (alist, const char *);
  if (mailbox == NULL)
    return 0;
  mbox = malloc (sizeof (struct mbox));
  if (mbox == NULL)
    return 0;
  mbox->phrase = (phrase != NULL) ? strdup (phrase) : NULL;
  mbox->mailbox = strdup (mailbox);

  mbox->next = header->value;
  header->value = mbox;
  return 1;
}

/* Print header-name ": " mailbox "\r\n"
      or header-name ": \"" phrase "\" <" mailbox ">\r\n"
   ad nauseum. */
static void
print_cc (smtp_message_t message, struct rfc2822_header *header)
{
  struct mbox *mbox;

  assert (message != NULL && header != NULL);

  vconcatenate (&message->hdr_buffer, header->header, ": ", NULL);
  for (mbox = header->value; mbox != NULL; mbox = mbox->next)
    {
      if (mbox->phrase == NULL)
	vconcatenate (&message->hdr_buffer, mbox->mailbox, NULL);
      else
	vconcatenate (&message->hdr_buffer,
		      "\"", mbox->phrase, "\" <", mbox->mailbox, ">",
		      NULL);
      vconcatenate (&message->hdr_buffer,
		    (mbox->next != NULL) ? ",\r\n    " : "\r\n", NULL);
    }
}

/* As above but generate a default value from the recipient list.
 */
static void
print_to (smtp_message_t message, struct rfc2822_header *header)
{
  smtp_recipient_t recipient;

  assert (header != NULL);

  if (header->value != NULL)
    {
      print_cc (message, header);
      return;
    }

  /* TODO: implement line folding at white spaces */
  vconcatenate (&message->hdr_buffer, header->header, ": ", NULL);
  for (recipient = message->recipients;
       recipient != NULL;
       recipient = recipient->next)
    vconcatenate (&message->hdr_buffer, recipient->mailbox,
		  (recipient->next != NULL) ? ",\r\n	" : "\r\n",
		  NULL);
}


/* Header actions placed here to avoid the need for many akward forward
   declarations for set_xxx/print_xxx.	*/

static const struct header_actions header_actions[] =
  {
    /* This is the default header info for a simple string value.
     */
    { NULL,		OPTIONAL,
      set_string,	print_string,		destroy_string},
    /* A number of headers should be present in every message
     */
    { "Date",		REQUIRE,
      set_date,		print_date, NULL, },
    { "From",		REQUIRE | LISTVALUE,
      set_from,		print_from,		destroy_mbox_list, },
    /* Certain headers are added when a message is delivered and
       should not be present in a message being posted or which
       is in transit.  If present in the message they will be stripped
       and if specified by the API, the relevant APIs will fail. */
    { "Return-Path",	PROHIBIT, NULL, NULL, NULL, },
    /* RFC 3798 section 2.3
    		- Delivering MTA may add an Original-Recipient: header
                  from the DSN ORCPT parameter and may discard any
		  Original-Recipient: headers present in the message.
		  No point in sending it then. */
    { "Original-Recipient", PROHIBIT, NULL, NULL, NULL, },
    /* MIME-*: and Content-*: are MIME headers and must not be generated
       or processed by libESMTP.  Similarly, Resent-*: and Received: must
       be retained unaltered. */
    { "Content-",	PRESERVE, NULL, NULL, NULL, },
    { "MIME-",		PRESERVE, NULL, NULL, NULL, },
    { "Resent-",	PRESERVE, NULL, NULL, NULL, },
    { "Resent-Reply-To", PROHIBIT, NULL, NULL, NULL, },
    { "Received",	PRESERVE, NULL, NULL, NULL, },
    /* Headers which are optional but which are recommended to be
       present.  Default action is to provide a default unless the
       application explicitly requests not to. */
    { "Message-Id",	SHOULD,
      set_string_null,print_message_id,		destroy_string, },
    /* Remaining headers are known to libESMTP to simplify handling them
       for the application.   All other headers are reaated as simple
       string values. */
    { "Sender",		OPTIONAL,
      set_sender,	print_sender,		destroy_mbox_list, },
    { "To",		OPTIONAL | LISTVALUE,
      set_to,		print_to,		destroy_mbox_list, },
    { "Cc",		OPTIONAL | LISTVALUE,
      set_cc,		print_cc,		destroy_mbox_list, },
    { "Bcc",		OPTIONAL | LISTVALUE,
      set_cc,		print_cc,		destroy_mbox_list, },
    { "Reply-To",	OPTIONAL | LISTVALUE,
      set_cc,		print_cc,		destroy_mbox_list, },
    /* RFC 3798 - MDN request.  Syntax is the same as the From: header and
		  default when set to NULL is the same as From: */
    { "Disposition-Notification-To", OPTIONAL,
      set_from,		print_from,		destroy_mbox_list, },
    /* TODO:
       In-Reply-To:	*(phrase / msgid)
       References:	*(phrase / msgid)
       Keywords:	#phrase

       Handle Resent- versions of
		To Cc Bcc Message-ID Date Reply-To From Sender
     */
  };

static int
init_header_table (smtp_message_t message)
{
  int i;
  struct header_info *hi;

  assert (message != NULL);

  if (message->hdr_action != NULL)
    return -1;

  message->hdr_action = h_create ();
  if (message->hdr_action == NULL)
    return 0;
  for (i = 0; i < NELT (header_actions); i++)
    if (header_actions[i].name != NULL)
      {
	hi = h_insert (message->hdr_action, header_actions[i].name, -1,
		       sizeof (struct header_info));
	if (hi == NULL)
	  return 0;
	hi->action = &header_actions[i];

	/* REQUIREd headers must be present in the message.  SHOULD
	   means the header is optional but its presence is recommended.
	   Create a NULL valued header.  This will either be set later
	   with the API, or the print_xxx function will handle the NULL
	   value as a special case, e.g, the To: header is generated
	   from the recipient_t list. */
	if (hi->action->flags & (REQUIRE | SHOULD))
	  {
	    if (create_header (message, header_actions[i].name, hi) == NULL)
	      return 0;
	  }
      }
  return 1;
}

void
destroy_header_table (smtp_message_t message)
{
  struct rfc2822_header *header, *next;

  assert (message != NULL);

  /* Take out the linked list */
  for (header = message->headers; header!= NULL; header = next)
    {
      next = header->next;
      if (header->info->action->destroy != NULL)
	(*header->info->action->destroy) (header);
      free (header->header);
      free (header);
    }

  /* Take out the hash table */
  if (message->hdr_action != NULL)
    {
      h_destroy (message->hdr_action, NULL, NULL);
      message->hdr_action = NULL;
    }

  message->headers = message->end_headers = NULL;
}

struct header_info *
find_header (smtp_message_t message, const char *name, int len)
{
  struct header_info *info;
  const char *p;

  assert (message != NULL && name != NULL);

  if (len < 0)
    len = strlen (name);
  if (len == 0)
    return NULL;
  info = h_search (message->hdr_action, name, len);
  if (info == NULL && (p = memchr (name, '-', len)) != NULL)
    info = h_search (message->hdr_action, name, p - name + 1);
  return info;
}

struct header_info *
insert_header (smtp_message_t message, const char *name)
{
  struct header_info *info;

  assert (message != NULL && name != NULL);

  info = h_insert (message->hdr_action, name, -1, sizeof (struct header_info));
  if (info == NULL)
    return NULL;
  info->action = &header_actions[0];
  return info;
}

static struct rfc2822_header *
create_header (smtp_message_t message, const char *header,
	       struct header_info *info)
{
  struct rfc2822_header *hdr;

  assert (message != NULL && header != NULL && info != NULL);

  if ((hdr = malloc (sizeof (struct rfc2822_header))) == NULL)
    return NULL;

  memset (hdr, 0, sizeof (struct rfc2822_header));
  hdr->header = strdup (header);
  hdr->info = info;
  info->hdr = hdr;
  APPEND_LIST (message->headers, message->end_headers, hdr);
  return hdr;
}

/****************************************************************************
 * Header processing
 ****************************************************************************/

/* Called just before copying the messge from the application.
   Resets the seen flag for headers libESMTP is interested in */

static void
reset_headercb (const char *name __attribute__ ((unused)),
		void *data, void *arg __attribute__ ((unused)))
{
  struct header_info *info = data;

  assert (info != NULL);

  info->seen = 0;
}

int
reset_header_table (smtp_message_t message)
{
  int status;

  assert (message != NULL);

  if ((status = init_header_table (message)) < 0)
    h_enumerate (message->hdr_action, reset_headercb, NULL);
  return status;
}

/* This is called to process headers present in the application supplied
   message.  */
const char *
process_header (smtp_message_t message, const char *header, int *len)
{
  const char *p;
  struct header_info *info;
  const struct header_actions *action;
  hdrprint_t print;

  assert (message != NULL && header != NULL && len != NULL);

  if (*len > 0
      && (p = memchr (header, ':', *len)) != NULL
      && (info = find_header (message, header, p - header)) != NULL)
    {
      if ((action = info->action) != NULL)
	{
	  //XXX is this true of RFC 5322?
	  /* RFC 2822 states that headers may only appear once in a
	     message with the exception of a few special headers.
	     This restriction is enforced here. */
	  if (info->seen && !(action->flags & (MULTIPLE | PRESERVE)))
	    header = NULL;
	  if (info->prohibit || (action->flags & PROHIBIT))
	    header = NULL;

	  /* When libESMTP is overriding headers in the message with
	     ones supplied in the API, the substitution is done here
	     to preserve the original ordering of the headers.	*/
	  if (header != NULL && info->override)
	    {
	      if ((print = action->print) == NULL)
		print = print_string;
	      cat_reset (&message->hdr_buffer, 0);
	      (*print) (message, info->hdr);
	      header = cat_buffer (&message->hdr_buffer, len);
	    }
	}
      else if (info->seen)
	header = NULL;
      info->seen = 1;
    }
  return header;
}

/* This is called to supply headers not present in the application supplied
   message.  */
const char *
missing_header (smtp_message_t message, int *len)
{
  struct header_info *info;
  hdrprint_t print;

  assert (message != NULL && len != NULL);

  /* Move on to the next header */
  if (message->current_header == NULL)
    message->current_header = message->headers;
  else
    message->current_header = message->current_header->next;

  /* Look for the next header that is actually required */
  print = NULL;
  while (message->current_header != NULL)
    {
      info = message->current_header->info;
      if (info == NULL)		/* shouldn't happen */
	break;
      if (!info->seen)
	{
	  if (info->action != NULL)
	    print = info->action->print;
	  break;
	}
      message->current_header = message->current_header->next;
    }
  if (message->current_header == NULL)
    {
      /* Free the buffer created by concatenate() and return NULL to
	 mark the end of the headers */
      cat_free (&message->hdr_buffer);
      return NULL;
    }

  if (print == NULL)
    print = print_string;

  cat_reset (&message->hdr_buffer, 0);
  (*print) (message, message->current_header);
  return cat_buffer (&message->hdr_buffer, len);
}

/****************************************************************************
 * Header API
 ****************************************************************************/

/**
 * DOC: Headers
 *
 * Message Headers
 * ---------------
 *
 * libESMTP provides a simple header API.  This is provided for two purposes
 * firstly to ensure that a message conforms to RFC 5322 when transferred to
 * the Mail Submission Agent (MSA) and, secondly, to simplify the application
 * logic where this is convenient.
 * 
 * Message headers known to libESMTP are listed below.  When setting arbitrary
 * headers, names should be prefixed with ``X-`` if non-standard.
 *
 * All newly submitted mail should have a unique Message-Id header. If the
 * message does not provide one and no value is supplied by smtp_set_header()
 * libESMTP will generate a suitable header unless the header option is changed.
 * 
 * List Values
 * ===========
 * 
 * Headers which accept lists are created by calling smtp_set_header() multiple
 * times for each value.
 * 
 * * From:
 * * To:
 * * Cc:
 * * Bcc:
 * * Reply-To:
 * 
 * Required Headers
 * ================
 * 
 * The following headers are required and are supplied by libESMTP if not
 * present in the message.
 * 
 * * Date:
 * * From:
 * 
 * Preserved Headers
 * =================
 * 
 * Preserved headers may not be specified using the libESMTP API, however if
 * present in the message they are copied unchanged. This is required to
 * correctly process MIME formatted messages, Resent and Received headers.
 * 
 * * Content-\*:
 * * MIME-\*:
 * * Resent-\*:
 * * Received:
 * 
 * Prohibited Headers
 * ==================
 * 
 * Prohibited may not be present in newly submitted messages and are stripped
 * from the message, if present.
 * 
 * * Return-Path:
 * * Original-Recipient:
 */

/**
 * smtp_set_header() - Set a message header
 * @message: The message
 * @header: Header name
 * @...: additional arguments
 *
 * Set a message header to the specified value.  The additional arguments
 * and their types depend on the header, detailed below.
 *
 * +-----------------------------+--------------------------------------------+
 * | Date                        | struct tm \*tm                             |
 * +                             +--------------------------------------------+
 * |                             | Message timestamp.                         |
 * +-----------------------------+--------------------------------------------+
 * | From                        | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Message originator.                        |
 * +-----------------------------+--------------------------------------------+
 * | Sender                      | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Message sent from address on behalf of     |
 * |                             | From mailbox.                              |
 * +-----------------------------+--------------------------------------------+
 * | To                          | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Primary recipient mailbox.                 |
 * +-----------------------------+--------------------------------------------+
 * | Cc                          | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Carbon-copy mailbox.                       |
 * +-----------------------------+--------------------------------------------+
 * | Bcc                         | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Blind carbon-copy mailbox.                 |
 * +-----------------------------+--------------------------------------------+
 * | Reply-To                    | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Reply to mailbox instead of From.          |
 * +-----------------------------+--------------------------------------------+
 * | Disposition-Notification-To | const char \*name, const char \*address    |
 * +                             +--------------------------------------------+
 * |                             | Return MDN to specified address.           |
 * +-----------------------------+--------------------------------------------+
 * | Message-Id                  | const char \*id                            |
 * |                             |                                            |
 * +                             +--------------------------------------------+
 * |                             | Message ID string or NULL.                 |
 * +-----------------------------+--------------------------------------------+
 * | \*                          | const char \*text                          |
 * +                             +--------------------------------------------+
 * |                             | Set arbitrary header. Value may not be NULL|
 * +-----------------------------+--------------------------------------------+
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_set_header (smtp_message_t message, const char *header, ...)
{
  va_list alist;
  struct rfc2822_header *hdr;
  struct header_info *info;
  hdrset_t set;

  SMTPAPI_CHECK_ARGS (message != NULL && header != NULL, 0);

  if (!init_header_table (message))
    {
      set_errno (ENOMEM);
      return 0;
    }

  info = find_header (message, header, -1);
  if (info == NULL && (info = insert_header (message, header)) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }

  /* Cannot specify a value for headers that must pass unchanged (MIME)
     or which may not appear in a posted message (Return-Path:).  */
  if (info->prohibit || (info->action->flags & (PROHIBIT | PRESERVE)))
    {
      set_error (SMTP_ERR_INVAL);
      return 0;
    }

  set = info->action->set;
  if (set == NULL)
    {
      set_error (SMTP_ERR_INVAL);
      return 0;
    }

  if (info->hdr == NULL)
    hdr = create_header (message, header, info);
  else if (info->hdr->value == NULL)
    hdr = info->hdr;
  else
    {
      /* Header has a previous value.  If multiple headers are permitted,
	 create a new value.  If the header has a list value, the value
	 is appended to the iost.  If neither condition applies, this
	 is an error. */
      if (info->action->flags & MULTIPLE)
	hdr = create_header (message, header, info);
      else if (info->action->flags & LISTVALUE)
	hdr = info->hdr;
      else
	{
	  set_error (SMTP_ERR_INVAL);
	  return 0;
	}
  }

  /* Set its value */
  va_start (alist, header);
  (*set) (hdr, alist);
  va_end (alist);

  return 1;
}

/**
 * smtp_set_header_option() - Set a message header
 * @message: The message
 * @header: Header name
 * @option: Header options
 * @...: additional arguments
 *
 * Set a message header option to the specified value.  The additional
 * arguments depend on the option as follows
 *
 * +--------------+-----------+-----------------------------------------+
 * | Option       | Arguments | Description                             |
 * +==============+===========+=========================================+
 * | Hdr_OVERRIDE | int       | non-zero to enable libESMTP to override |
 * |              |           | header in message.                      |
 * +--------------+-----------+-----------------------------------------+
 * | Hdr_PROHIBIT | int       | non-zero to force libESMTP to strip     |
 * |              |           | header from message.                    |
 * +--------------+-----------+-----------------------------------------+
 *
 * It is not possible to change the header options for required, prohibited
 * or preserved headers listed in the introduction.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_set_header_option (smtp_message_t message, const char *header,
			enum header_option option, ...)
{
  va_list alist;
  struct header_info *info;

  SMTPAPI_CHECK_ARGS (message != NULL && header != NULL, 0);

  if (!init_header_table (message))
    {
      set_errno (ENOMEM);
      return 0;
    }

  info = find_header (message, header, -1);
  if (info == NULL && (info = insert_header (message, header)) == NULL)
    {
      set_errno (ENOMEM);
      return 0;
    }

  /* Don't permit options to be set on headers that must pass intact or
     which are prohibited. */
  if (info->action->flags & (PROHIBIT | PRESERVE))
    {
      set_error (SMTP_ERR_INVAL);
      return 0;
    }

  /* There is an odd quirk when setting options.  Setting an option for
     the OPTIONAL headers known to libESMTP causes default values to be
     generated automatically when not found in the message, so long as
     there is no other reason to prevent them appearing in the message! */

  /* Don't allow the user to set override on prohibited headers. */
  if (option == Hdr_OVERRIDE && !info->prohibit)
    {
      va_start (alist, option);
      info->override = !!va_arg (alist, int);
      va_end (alist);
      return 1;
    }
  /* Don't allow the user to prohibit required headers. */
  if (option == Hdr_PROHIBIT && !(info->action->flags & REQUIRE))
    {
      va_start (alist, option);
      info->prohibit = !!va_arg (alist, int);
      va_end (alist);
      return 1;
    }

  set_error (SMTP_ERR_INVAL);
  return 0;
}

int
smtp_set_resent_headers (smtp_message_t message, int onoff)
{
  SMTPAPI_CHECK_ARGS (message != NULL, 0);

  /* TODO: place holder, implement real functionality here.
	   For now, succeed if the onoff argument is zero. */
  SMTPAPI_CHECK_ARGS (onoff == 0, 0);

  return 1;
}
