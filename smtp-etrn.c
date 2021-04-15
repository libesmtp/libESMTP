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

#ifdef USE_ETRN
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <missing.h> /* declarations for missing library functions */

#include "libesmtp-private.h"

#include "siobuf.h"
#include "protocol.h"
#include "attribute.h"

/**
 * DOC: RFC 1985
 *
 * Remote Message Queue Starting (ETRN)
 * ------------------------------------
 *
 * The SMTP ETRN extension is used to request a remote MTA to start its
 * delivery queue for the specified domain.  If the application requests
 * the use if the ETRN extension and the remote MTA does not list ETRN,
 * libESMTP will use the event callback to notify the application.
 */

struct smtp_etrn_node
  {
    struct smtp_etrn_node *next;
    struct smtp_session *session;	/* Back reference */

  /* Application data */
    void *application_data;		/* Pointer to data maintained by app */
    void (*release) (void *);		/* function to free/unref data */

  /* Node Info */
    int option;				/* Option character */
    char *domain;			/* Remote queue to start */

  /* Status */
    smtp_status_t status;		/* Status from MTA greeting */
  };

/**
 * smtp_etrn_add_node() - create an ETRN node.
 * @session: The session.
 * @option: The option character.
 * @domain: Request mail for this domain.
 *
 * Add an ETRN node to the SMTP session.
 *
 * Return: An &typedef smtp_etrn_node_t or %NULL on failure.
 */
smtp_etrn_node_t
smtp_etrn_add_node (smtp_session_t session, int option, const char *domain)
{
  smtp_etrn_node_t node;
  char *dup_domain;

  SMTPAPI_CHECK_ARGS (session != NULL
		      && domain != NULL
		      && (option == 0 || option == '@'), NULL);

  if ((node = malloc (sizeof (struct smtp_etrn_node))) == NULL)
    {
      set_errno (ENOMEM);
      return NULL;
    }
  if ((dup_domain = strdup (domain)) == NULL)
    {
      free (node);
      set_errno (ENOMEM);
      return NULL;
    }

  memset (node, 0, sizeof (struct smtp_etrn_node));
  node->session = session;
  node->option = option;
  node->domain = dup_domain;
  APPEND_LIST (session->etrn_nodes, session->end_etrn_nodes, node);
  session->required_extensions |= EXT_ETRN;
  return node;
}

/**
 * smtp_etrn_enumerate_nodes() - Call function for each ETRN node in session.
 * @session: The session.
 * @cb: Callback function
 * @arg: Argument (closure) passed to callback.
 *
 * Call the callback function once for each ETRN node in the SMTP session.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
smtp_etrn_enumerate_nodes (smtp_session_t session,
			   smtp_etrn_enumerate_nodecb_t cb, void *arg)
{
  smtp_etrn_node_t node;

  SMTPAPI_CHECK_ARGS (session != NULL && cb != NULL, 0);

  for (node = session->etrn_nodes; node != NULL; node = node->next)
    (*cb) (node, node->option, node->domain, arg);
  return 1;
}

/**
 * smtp_etrn_node_status() - Retrieve ETRN status.
 * @node: An &smtp_etrn_node_t returned by smtp_etrn_add_node()
 *
 * Retrieve the ETRN node success/failure status from a previous SMTP session.
 * This includes SMTP status codes, RFC 2034 enhanced status codes, if
 * available and text from the server describing the status.
 *
 * Return: %NULL if no status information is available,
 * otherwise a pointer to the status information.  The pointer remains valid
 * until the next call to libESMTP in the same thread.
 */
const smtp_status_t *
smtp_etrn_node_status (smtp_etrn_node_t node)
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return &node->status;
}

/**
 * smtp_etrn_set_application_data() - Associate data with an ETRN node.
 * @node: An &smtp_etrn_node_t returned by smtp_etrn_add_node()
 * @data: Application data to set
 *
 * Associate application defined data with the opaque ETRN structure.
 *
 * Only available when %LIBESMTP_ENABLE_DEPRECATED_SYMBOLS is defined.
 * Use smtp_etrn_set_application_data_release() instead.
 *
 * Return: Previously set application data or %NULL.
 */
void *
smtp_etrn_set_application_data (smtp_etrn_node_t node, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  old = node->application_data;
  node->application_data = data;
  node->release = NULL;
  return old;
}

/**
 * smtp_etrn_set_application_data_release() - Associate data with an ETRN node.
 * @node: An &smtp_etrn_node_t returned by smtp_etrn_add_node()
 * @data: Application data
 * @release: function to free/unref data.
 *
 * Associate application data with the ETRN node.
 * If @release is non-NULL it is called to free or unreference data when
 * changed or the session is destroyed.
 */
void
smtp_etrn_set_application_data_release (smtp_etrn_node_t node, void *data,
				        void (*release) (void *))
{
  SMTPAPI_CHECK_ARGS (node != NULL, /* void */);

  if (node->application_data != NULL && node->release != NULL)
    (*node->release) (node->application_data);

  node->release = release;
  node->application_data = data;
}

/**
 * smtp_etrn_get_application_data() - Get data from an ETRN node.
 * @node: An &smtp_etrn_node_t returned by smtp_etrn_add_node()
 *
 * Retrieve application data from the opaque ETRN structure.
 *
 * Return: Application data or %NULL.
 */
void *
smtp_etrn_get_application_data (smtp_etrn_node_t node)
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return node->application_data;
}

int
check_etrn (smtp_session_t session)
{
  return (session->extensions & EXT_ETRN) && session->etrn_nodes != NULL;
}

void
destroy_etrn_nodes (smtp_session_t session)
{
  smtp_etrn_node_t node, next;

  for (node = session->etrn_nodes; node != NULL; node = next)
    {
      next = node->next;

      if (node->application_data != NULL && node->release != NULL)
	(*node->release) (node->application_data);
      free (node->domain);
      free (node);
    }
  session->etrn_nodes = session->end_etrn_nodes = NULL;
  session->cmd_etrn_node = session->rsp_etrn_node = NULL;
}

void
cmd_etrn (siobuf_t conn, smtp_session_t session)
{
  smtp_etrn_node_t node;

  if (session->cmd_etrn_node == NULL)
    session->cmd_etrn_node = session->etrn_nodes;
  node = session->cmd_etrn_node;

  sio_printf (conn, "ETRN %c%s\r\n",
	      node->option ? node->option : ' ', node->domain);

  session->cmd_etrn_node = session->cmd_etrn_node->next;
  if (session->cmd_etrn_node != NULL)
    session->cmd_state = S_etrn;
  else if (session->cmd_recipient != NULL)
    session->cmd_state = initial_transaction_state (session);
  else
    session->cmd_state = S_quit;
}

void
rsp_etrn (siobuf_t conn, smtp_session_t session)
{
  int code;
  smtp_etrn_node_t node;

  if (session->rsp_etrn_node == NULL)
    session->rsp_etrn_node = session->etrn_nodes;
  node = session->rsp_etrn_node;
  code = read_smtp_response (conn, session, &node->status, NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }

  /* Notify the ETRN status */
  if (session->event_cb != NULL)
    (*session->event_cb) (session, SMTP_EV_ETRNSTATUS, session->event_cb_arg,
			  node->option, node->domain);

  session->rsp_etrn_node = session->rsp_etrn_node->next;
  if (session->rsp_etrn_node != NULL)
    session->rsp_state = S_etrn;
  else if (session->rsp_recipient != NULL)
    session->rsp_state = initial_transaction_state (session);
  else
    session->rsp_state = S_quit;
}

#else

#include <stdlib.h>
#include <errno.h>

#include "libesmtp-private.h"

smtp_etrn_node_t
smtp_etrn_add_node (smtp_session_t session, int option, const char *domain)
{
  SMTPAPI_CHECK_ARGS (session != NULL
		      && domain != NULL
		      && (option == 0 || option == '@'), NULL);

  return NULL;
}

int
smtp_etrn_enumerate_nodes (smtp_session_t session,
			   smtp_etrn_enumerate_nodecb_t cb,
			   void *arg __attribute__ ((unused)))
{
  SMTPAPI_CHECK_ARGS (session != NULL && cb != NULL, 0);

  return 0;
}

const smtp_status_t *
smtp_etrn_node_status (smtp_etrn_node_t node)
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return NULL;
}

void *
smtp_etrn_set_application_data (smtp_etrn_node_t node,
				void *data __attribute__ ((unused)))
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return NULL;
}

void
smtp_etrn_set_application_data_release (smtp_etrn_node_t node
						      __attribute__ ((unused)),
				        void *data __attribute__ ((unused)),
				        void (*release) (void *)
						      __attribute__ ((unused)))
{
}

void *
smtp_etrn_get_application_data (smtp_etrn_node_t node)
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return NULL;
}

#endif
