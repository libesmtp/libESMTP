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

#ifdef USE_PTHREADS
#include <pthread.h>
#endif
#include <ltdl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "auth-client.h"
#include "auth-plugin.h"
#include "api.h"

#ifdef USE_PTHREADS
static pthread_mutex_t plugin_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct auth_plugin
  {
    struct auth_plugin *next;
    lt_dlhandle module;
    const struct auth_client_plugin *info;
  };
static struct auth_plugin *client_plugins, *end_client_plugins;

struct auth_context
  {
    int min_ssf;
    unsigned flags;
    const struct auth_client_plugin *client;
    void *plugin_ctx;
    auth_interact_t interact;
    void *interact_arg;
    const char *external_id;
  };

#define mechanism_disabled(p,a,f)		\
	  (((p)->flags & AUTH_PLUGIN_##f) && !((a)->flags & AUTH_PLUGIN_##f))

static const char *
plugin_name (char *buf, int buflen, const char *str)
{
  char *p;
  static const char prefix[] = "sasl-";

  strcpy (buf, prefix);
  p = buf + sizeof prefix - 1;
  buflen -= sizeof prefix;
  while (*str != '\0' && buflen-- > 0)
    *p++ = tolower (*str++);
  *p = '\0';
  return buf;
}

static void
append_plugin (lt_dlhandle module, const struct auth_client_plugin *info)
{
  struct auth_plugin *auth_plugin;

  auth_plugin = malloc (sizeof (struct auth_plugin));
  auth_plugin->info = info;
  auth_plugin->module = module;
  auth_plugin->next = NULL;
  if (client_plugins == NULL)
    client_plugins = auth_plugin;
  else
    end_client_plugins->next = auth_plugin;
  end_client_plugins = auth_plugin;
}

static const struct auth_client_plugin *
load_client_plugin (auth_context_t context, const char *name)
{
  lt_dlhandle module;
  char buf[32];
  const char *plugin;
  const struct auth_client_plugin *info;

  /* Try searching for a plugin. */
  plugin = plugin_name (buf, sizeof buf, name);
  module = lt_dlopenext (plugin);
  if (module == NULL)
    return 0;

  info = lt_dlsym (module, "sasl_client");
  if (info == NULL)
    {
      lt_dlclose (module);
      return NULL;
    }

  /* Check the application's requirements regarding minimum SSF and
     whether anonymous or plain text mechanisms are explicitly allowed. */
  if (info->ssf < context->min_ssf
      || mechanism_disabled (info, context, EXTERNAL)
      || mechanism_disabled (info, context, ANONYMOUS)
      || mechanism_disabled (info, context, PLAIN))
    {
      lt_dlclose (module);
      return NULL;
    }

  /* This plugin module is OK.  Add it to the list of loaded modules.
   */
  append_plugin (module, info);
  return info;
}

void
auth_client_init (void)
{
#ifdef USE_PTHREADS
  pthread_mutex_lock (&plugin_mutex);
#endif
  lt_dlinit ();
#ifdef AUTHPLUGINDIR
  lt_dladdsearchdir (AUTHPLUGINDIR);
#endif
  /* Add builtin mechanisms to the plugin list */
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&plugin_mutex);
#endif
}

void
auth_client_exit (void)
{
  struct auth_plugin *plugin, *next;

  /* Scan the auth_plugin array and lt_dlclose() the modules */
#ifdef USE_PTHREADS
  pthread_mutex_lock (&plugin_mutex);
#endif
  for (plugin = client_plugins; plugin != NULL; plugin = next)
    {
      next = plugin->next;
      if (plugin->module != NULL)
	lt_dlclose (plugin->module);
      free (plugin);
    }
  client_plugins = end_client_plugins = NULL;
  lt_dlexit ();
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&plugin_mutex);
#endif
}

auth_context_t
auth_create_context (void)
{
  auth_context_t context;

  context = malloc (sizeof (struct auth_context));
  if (context == NULL)
    return NULL;

  memset (context, 0, sizeof (struct auth_context));
  return context;
}

int
auth_destroy_context (auth_context_t context)
{
  API_CHECK_ARGS (context != NULL, 0);

  if (context->plugin_ctx != NULL)
    {
      if (context->client != NULL && context->client->destroy != NULL)
	(*context->client->destroy) (context->plugin_ctx);
    }
  free (context);
  return 1;
}

int
auth_set_mechanism_flags (auth_context_t context, unsigned set, unsigned clear)
{
  API_CHECK_ARGS (context != NULL, 0);

  context->flags &= AUTH_PLUGIN_EXTERNAL | ~clear;
  context->flags |= ~AUTH_PLUGIN_EXTERNAL & set;
  return 1;
}

int
auth_set_mechanism_ssf (auth_context_t context, int min_ssf)
{
  API_CHECK_ARGS (context != NULL, 0);

  context->min_ssf = min_ssf;
  return 1;
}

int
auth_set_external_id (auth_context_t context, const char *identity)
{
  static const struct auth_client_plugin external_client =
    {
    /* Plugin information */
      "EXTERNAL",
      "SASL EXTERNAL mechanism (RFC 2222)",
    /* Plugin instance */
      NULL,
      NULL,
    /* Authentication */
      NULL,
      AUTH_PLUGIN_EXTERNAL,
    /* Security Layer */
      0,
      NULL,
      NULL,
    };
  struct auth_plugin *plugin;

  API_CHECK_ARGS (context != NULL, 0);

  if (context->external_id != NULL)
    free ((void *) context->external_id);
  if (identity != NULL)
    {
      /* Install external module if required */
      for (plugin = client_plugins; plugin != NULL; plugin = plugin->next)
        if (plugin->info->flags & AUTH_PLUGIN_EXTERNAL)
          break;
      if (plugin == NULL)
        {
#ifdef USE_PTHREADS
	  pthread_mutex_lock (&plugin_mutex);
#endif
	  append_plugin (NULL, &external_client);
#ifdef USE_PTHREADS
	  pthread_mutex_unlock (&plugin_mutex);
#endif
	}
      context->flags |= AUTH_PLUGIN_EXTERNAL;
      context->external_id = strdup (identity);
    }
  else
    {
      context->flags &= ~AUTH_PLUGIN_EXTERNAL;
      context->external_id = NULL;
    }
  return 1;
}

int
auth_set_interact_cb (auth_context_t context,
		      auth_interact_t interact, void *arg)
{
  API_CHECK_ARGS (context != NULL, 0);

  context->interact = interact;
  context->interact_arg = arg;
  return 1;
}

/* Perform various checks to see if SASL is usable.   Do not check
   for loaded plugins though.  This is checked when trying to
   select one for use. */
int
auth_client_enabled (auth_context_t context)
{
  if (context == NULL)
    return 0;
  if (context->interact == NULL)
    return 0;
  return 1;
}

int
auth_set_mechanism (auth_context_t context, const char *name)
{
  struct auth_plugin *plugin;
  const struct auth_client_plugin *info;

  API_CHECK_ARGS (context != NULL, 0);

#ifdef USE_PTHREADS
  pthread_mutex_lock (&plugin_mutex);
#endif

  /* Get rid of old context */
  if (context->plugin_ctx != NULL)
    {
      if (context->client != NULL && context->client->destroy != NULL)
	(*context->client->destroy) (context->plugin_ctx);
      context->plugin_ctx = NULL;
    }

  /* Check the list of already loaded modules. */
  info = NULL;
  for (plugin = client_plugins; plugin != NULL; plugin = plugin->next)
    if (strcasecmp (name, plugin->info->keyw) == 0)
      {
	info = plugin->info;
	break;
      }

  if (info != NULL)
    {
      /* Check the application's requirements regarding minimum SSF and
	 whether anonymous or plain text mechanisms are explicitly
	 allowed.  This has to be checked here since the list of loaded
	 plugins is global and the plugin may have been acceptable in
	 another context but not in this one.  */
      if (info->ssf < context->min_ssf
	  || mechanism_disabled (info, context, EXTERNAL)
	  || mechanism_disabled (info, context, ANONYMOUS)
	  || mechanism_disabled (info, context, PLAIN))
	{
#ifdef USE_PTHREADS
	  pthread_mutex_unlock (&plugin_mutex);
#endif
	  return 0;
	}
    }
  else if ((info = load_client_plugin (context, name)) == NULL)
    {
#ifdef USE_PTHREADS
      pthread_mutex_unlock (&plugin_mutex);
#endif
      return 0;
    }

  context->client = info;
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&plugin_mutex);
#endif
  return 1;
}

const char *
auth_response (auth_context_t context, const char *challenge, int *len)
{
  API_CHECK_ARGS (context != NULL && context->client != NULL && len != NULL, NULL);

  if (challenge == NULL)
    {
      if (context->plugin_ctx != NULL && context->client->destroy != NULL)
        (*context->client->destroy) (context->plugin_ctx);
      if (context->client->init == NULL)
	context->plugin_ctx = NULL;
      else if (!(*context->client->init) (&context->plugin_ctx))
	return NULL;
    }
  if (context->client->flags & AUTH_PLUGIN_EXTERNAL)
    {
      *len = strlen (context->external_id);
      return context->external_id;
    }
  return (*context->client->response) (context->plugin_ctx,
  				       challenge, len,
				       context->interact,
				       context->interact_arg);
}

int
auth_get_ssf (auth_context_t context)
{
  API_CHECK_ARGS (context != NULL && context->client != NULL, -1);

  return context->client->ssf;
}

void
auth_encode (char **dstbuf, int *dstlen,
             const char *srcbuf, int srclen, void *arg)
{
  auth_context_t context = arg;

  if (context != NULL
      && context->client != NULL
      && context->client->encode != NULL)
    (*context->client->encode) (context->plugin_ctx,
				dstbuf, dstlen, srcbuf, srclen);
}

void
auth_decode (char **dstbuf, int *dstlen,
	     const char *srcbuf, int srclen, void *arg)
{
  auth_context_t context = arg;

  if (context != NULL
      && context->client != NULL
      && context->client->encode != NULL)
    (*context->client->decode) (context->plugin_ctx,
				dstbuf, dstlen, srcbuf, srclen);
}
