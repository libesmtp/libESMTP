/*
 *  This file is part of libESMTP, a library for submission of RFC 822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 821.
 *
 *  Copyright (C) 2001  Brian Stafford  <brian@stafford.uklinux.net>
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "libesmtp-private.h"
#include "headers.h"
#include "htable.h"
#include "rfc822date.h"
#include "api.h"

struct rfc822_header
  {
    struct rfc822_header *next;
    struct header_info *info;		/* Info for setting and printing */
    char *header;			/* Header name */
    void *value;			/* Header value */
  };

typedef int (*hdrset_t) (struct rfc822_header *, va_list);
typedef void (*hdrprint_t) (smtp_message_t, struct rfc822_header *);
typedef void (*hdrdestroy_t) (struct rfc822_header *);

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
    struct rfc822_header *hdr;	/* Pointer to most recently defined header */
    unsigned int seen : 1;	/* Header has been seen in the message */
    unsigned int override : 1;	/* LibESMTP is overriding the message */
    unsigned int prohibit : 1;	/* Header may not appear in the message */
  };

#define NELT(x)		((int) (sizeof x / sizeof x[0]))

#define OPTIONAL	0
#define REQUIRE		1
#define PROHIBIT	2
#define PRESERVE	4
#define ONCEONLY	8

static struct rfc822_header *create_header (smtp_message_t message,
					    const char *header,
					    struct header_info *info);
void destroy_string (struct rfc822_header *header);
void destroy_mbox_list (struct rfc822_header *header);
struct header_info *find_header (smtp_message_t message,
				 const char *name, int len);
struct header_info *insert_header (smtp_message_t message, const char *name);

/* RFC 822 headers processing */

/****************************************************************************
 * Functions for setting and printing header values
 ****************************************************************************/

static int
set_string (struct rfc822_header *header, va_list alist)
{
  const char *value;

  if (header->value != NULL)		/* Already set */
    return 0;

  value = va_arg (alist, const char *);
  if (value == NULL)
    return 0;
  header->value = strdup (value);
  return header->value != NULL;
}

static int
set_string_null (struct rfc822_header *header, va_list alist)
{
  const char *value;

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
print_string (smtp_message_t message, struct rfc822_header *header)
{
  /* TODO: implement line folding at white spaces */
  vconcatenate (&message->hdr_buffer, header->header, ": ",
                (header->value != NULL) ? header->value : "", "\r\n", NULL);
}

void
destroy_string (struct rfc822_header *header)
{
  if (header->value != NULL)
    free (header->value);
}

/* Print header-name ": <" message-id ">\r\n" */
static void
print_message_id (smtp_message_t message, struct rfc822_header *header)
{
  const char *message_id = header->value;
  char buf[64];
  static int generation;

  if (message_id == NULL)
    {
      snprintf (buf, sizeof buf, "%ld.%d@%s",
		time (NULL), generation++, message->session->localhost);
      message_id = buf;
    }
  /* TODO: implement line folding at white spaces */
  vconcatenate (&message->hdr_buffer,
                header->header, ": <", message_id, ">\r\n",
                NULL);
}

/****/

static int
set_date (struct rfc822_header *header, va_list alist)
{
  const time_t *value;

  if ((time_t) header->value != (time_t) 0)		/* Already set */
    return 0;

  value = va_arg (alist, const time_t *);
  header->value = (void *) *value;
  return 1;
}

/* Print header-name ": " formatted-date "\r\n" */
static void
print_date (smtp_message_t message, struct rfc822_header *header)
{
  char buf[64];
  time_t when;

  when = (time_t) header->value;
  if (when == (time_t) 0)
    time (&when);
  vconcatenate (&message->hdr_buffer, header->header, ": ",
                rfc822date (buf, sizeof buf, &when), "\r\n", NULL);
}

/****/

struct mbox
  {
    struct mbox *next;
    char *mailbox;
    char *phrase;
  };

void
destroy_mbox_list (struct rfc822_header *header)
{
  struct mbox *mbox, *next;

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
set_from (struct rfc822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

  phrase = va_arg (alist, const char *);
  mailbox = va_arg (alist, const char *);

  /* Allow this to succeed as a special case.  Effectively requesting
     default action in print_from().   Fails if explicit values have
     already been set.  */
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
print_from (smtp_message_t message, struct rfc822_header *header)
{
  struct mbox *mbox;
  const char *mailbox;

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
set_sender (struct rfc822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

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

  mbox->next = header->value;
  return 1;
}

/* Print header-name ": " mailbox "\r\n" 
      or header-name ": \"" phrase "\" <" mailbox ">\r\n"
 */
static void
print_sender (smtp_message_t message, struct rfc822_header *header)
{
  struct mbox *mbox;
  const char *mailbox;

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
set_to (struct rfc822_header *header, va_list alist)
{
  struct mbox *mbox;
  const char *mailbox;
  const char *phrase;

  phrase = va_arg (alist, const char *);
  mailbox = va_arg (alist, const char *);
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
print_cc (smtp_message_t message, struct rfc822_header *header)
{
  struct mbox *mbox;

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
print_to (smtp_message_t message, struct rfc822_header *header)
{
  smtp_recipient_t recipient;

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
		  (recipient->next != NULL) ? ",\r\n    " : "\r\n",
		  NULL);
}


/* Header actions placed here to avoid the need for many akward forward
   declarations for set_xxx/print_xxx.  */

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
    { "From",		REQUIRE,
      set_from,		print_from,		destroy_mbox_list, },
    { "To",		REQUIRE,
      set_to,		print_to,		destroy_mbox_list, },
    /* Certain headers are added when a message is delivered and
       should not be present in a message being posted or which
       is in transit.  If present in the message they will be stripped
       and if specified by the API, the relevant APIs will fail. */
    { "Return-Path",	PROHIBIT, NULL, NULL, NULL, },
    /* RFC 2298 - Delivering MTA may add the Original-Recipient: header
                  using DSN ORCPT parameter and may discard
                  Original-Recipient: headers present in the message.
                  No point in sending it then. */
    { "Original-Recipient", PROHIBIT, NULL, NULL, NULL, },
    /* MIME-*: and Content-*: are MIME headers and must not be generated
       or processed by libESMTP */
    { "Content-",	PRESERVE, NULL, NULL, NULL, },
    { "MIME-",		PRESERVE, NULL, NULL, NULL, },
    /* Remaining headers are known to libESMTP to simplify handling them
       for the application.   All other headers are reaated as simple
       string values. */
    { "Sender",		OPTIONAL,
      set_sender,	print_sender,		destroy_mbox_list, },
    { "Message-Id",	OPTIONAL,
      set_string_null,print_message_id,		destroy_string, },
    { "Cc",		OPTIONAL,
      set_to,		print_cc,		destroy_mbox_list, },
    { "Bcc",		OPTIONAL,
      set_to,		print_cc,		destroy_mbox_list, },
    { "Reply-To",	OPTIONAL,
      set_to,		print_cc,		destroy_mbox_list, },
    /* RFC 2298 - MDN request.  Syntax is the same as the From: header and
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
  struct h_node *node;
  struct header_info *hi;

  if (message->hdr_action != NULL)
    return -1;

  message->hdr_action = h_create ();
  if (message->hdr_action == NULL)
    return 0;
  for (i = 0; i < NELT (header_actions); i++)
    if (header_actions[i].name != NULL)
      {
	node = h_insert (message->hdr_action, header_actions[i].name, -1,
			 sizeof (struct header_info));
	if (node == NULL)
	  return 0;
	hi = h_dptr (node, struct header_info);
	hi->action = &header_actions[i];

	/* REQUIREd headers must be present in the message.  Create
	   a NULL valued header.  This will either be set later with
	   the API, or the print_xxx function will handle the NULL
	   value as a special case, e.g, the To: header is generated
	   from the recipient_t list. */
	if (hi->action->flags & REQUIRE)
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
  struct rfc822_header *header, *next;

  /* Take out the hash table */
  h_destroy (message->hdr_action, NULL, NULL);
  message->hdr_action = NULL;

  /* Take out the linked list */
  for (header = message->headers; header!= NULL; header = next)
    {
      next = header->next;
      if (header->info->action->destroy != NULL)
	(*header->info->action->destroy) (header);
      free (header->header);
      free (header);
    }
  message->headers = message->end_headers = NULL;
}

struct header_info *
find_header (smtp_message_t message, const char *name, int len)
{
  struct h_node *node;
  const char *p;

  node = h_search (message->hdr_action, name, len);
  if (node == NULL && (p = memchr (name, '-', len)) != NULL)
    node = h_search (message->hdr_action, name, p - name + 1);
  return (node != NULL) ? h_dptr (node, struct header_info) : NULL;
}

struct header_info *
insert_header (smtp_message_t message, const char *name)
{
  struct h_node *node;
  struct header_info *info;

  node = h_insert (message->hdr_action, name, -1, sizeof (struct header_info));
  if (node == NULL)
    return NULL;
  info = h_dptr (node, struct header_info);
  info->action = &header_actions[0];
  return info;
}

static struct rfc822_header *
create_header (smtp_message_t message, const char *header,
               struct header_info *info)
{
  struct rfc822_header *hdr;

  if ((hdr = malloc (sizeof (struct rfc822_header))) == NULL)
    return NULL;

  memset (hdr, 0, sizeof (struct rfc822_header));
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
reset_headercb (struct h_node *node, void *arg __attribute__((unused)))
{
  struct header_info *info = h_dptr (node, struct header_info);

  info->seen = 0;
}

int
reset_header_table (smtp_message_t message)
{
  int status;

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

  if ((p = memchr (header, ':', *len)) != NULL
      && (info = find_header (message, header, p - header)) != NULL)
    {
      /* In the case where libESMTP is overriding headers in the message
         with ones supplied in the API, the substitution could be done
         here so as to preserve the original ordering of the headers.
         For simplicity this is currently not done. */
      if ((action = info->action) != NULL)
        {
          if (info->prohibit || (action->flags & PROHIBIT))
            header = NULL;
          if ((action->flags & ONCEONLY) && info->seen)
            header = NULL;
          if (info->override)
            header = NULL;
        }
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
      if (!info->seen || info->override)
        {
	  if (info->action != NULL)
	    print = info->action->print;
	  break;
        }
      message->current_header = message->current_header->next;
    }
  if (message->current_header == NULL)
    {
      /* Free the buffer created by concatenate(). */
      cat_free (&message->hdr_buffer);
      /* Return NULL to mark the end of the headers */
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

int
smtp_set_header (smtp_message_t message, const char *header, ...)
{
  va_list alist;
  struct rfc822_header *hdr;
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

  /* Certain headers are not repeated, for example, To/Cc/Bcc.  Allow
     multiple calls to build up the value gradually.  For other headers
     create multiple instances of the header. */
  if (info->hdr != NULL		/* A previous header of this name exists */
      && (info->hdr->value == NULL	/* required but not defined */
          || (info->action->flags & ONCEONLY)))	/* supplement existing value */
    hdr = info->hdr;
  else
    hdr = create_header (message, header, info);

  /* Set its value */
  va_start (alist, header);
  (*set) (hdr, alist);
  va_end (alist);

  return 1;
}

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

  if (info->prohibit || (info->action->flags & (PROHIBIT | PRESERVE)))
    {
      set_error (SMTP_ERR_INVAL);
      return 0;
    }

  if (option == Hdr_OVERRIDE)
    {
      va_start (alist, option);
      info->override = !!va_arg (alist, int);
      va_end (alist);
      return 1;
    }
  if (option == Hdr_PROHIBIT)
    {
      /* Can't set prohibit if a header of this name already set up
         or the header is required. */
      if (info->hdr != NULL || (info->action->flags & REQUIRE))
        return 0;
      info->prohibit = 1;
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
