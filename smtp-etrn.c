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

#ifdef USE_ETRN
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <missing.h> /* declarations for missing library functions */

#include "libesmtp-private.h"

#include "siobuf.h"
#include "protocol.h"

struct smtp_etrn_node
  {
    struct smtp_etrn_node *next;
    struct smtp_session *session;	/* Back reference */

  /* Application data */
    void *application_data;		/* Pointer to data maintained by app */

  /* Node Info */
    int option;				/* Option character */
    char *domain;			/* Remote queue to start */

  /* Status */
    smtp_status_t status;		/* Status from MTA greeting */
  };

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
      return 0;
    }
  if ((dup_domain = strdup (domain)) == NULL)
    {
      free (node);
      set_errno (ENOMEM);
      return 0;
    }

  memset (node, 0, sizeof (struct smtp_etrn_node));
  node->session = session;
  node->option = option;
  node->domain = dup_domain;
  APPEND_LIST (session->etrn_nodes, session->end_etrn_nodes, node);
  session->required_extensions |= EXT_ETRN;
  return node;
}

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

const smtp_status_t *
smtp_etrn_node_status (smtp_etrn_node_t node)
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return &node->status;
}

void *
smtp_etrn_set_application_data (smtp_etrn_node_t node, void *data)
{
  void *old;

  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  old = node->application_data;
  node->application_data = data;
  return old;
}

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

void *
smtp_etrn_get_application_data (smtp_etrn_node_t node)
{
  SMTPAPI_CHECK_ARGS (node != NULL, NULL);

  return NULL;
}

#endif
