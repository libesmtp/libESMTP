/*
 *  This file is part of libESMTP, a library for submission of RFC 5322
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 4409 and RFC 5321.
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

/**
 * DOC: Auth Client
 *
 * Auth Client
 * -----------
 *
 * The auth client is a simple SASL implementation supporting the
 * SMTP AUTH extension.
 */

#include <config.h>

#include <assert.h>

#ifdef USE_PTHREADS
#include <pthread.h>
#endif

#include <dlfcn.h>
#ifndef DLEXT
#  define DLEXT ".so"
#endif
#ifndef RTLD_LAZY
#  define RTLD_LAZY RTLD_NOW
#endif

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <missing.h>

#include "auth-client.h"
#include "auth-plugin.h"
#include "api.h"

#ifdef USE_PTHREADS
static pthread_mutex_t plugin_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

struct auth_plugin
  {
    struct auth_plugin *next;
    void *module;
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
    char *external_id;
  };

#define mechanism_disabled(p,a,f)		\
	  (((p)->flags & AUTH_PLUGIN_##f) && !((a)->flags & AUTH_PLUGIN_##f))

#if defined AUTHPLUGINDIR
# define PLUGIN_DIR AUTHPLUGINDIR "/"
#else
# define PLUGIN_DIR
#endif

static char *
plugin_name (const char *str)
{
  char *buf, *p;
  static const char prefix[] = PLUGIN_DIR "sasl-";

  assert (str != NULL);

  buf = malloc (sizeof prefix + strlen (str) + sizeof DLEXT);
  if (buf == NULL)
    return NULL;
  strcpy (buf, prefix);
  p = buf + sizeof prefix - 1;
  while (*str != '\0')
    *p++ = tolower (*str++);
  strcpy (p, DLEXT);
  return buf;
}

static int
append_plugin (void *module, const struct auth_client_plugin *info)
{
  struct auth_plugin *auth_plugin;

  assert (info != NULL);

  auth_plugin = malloc (sizeof (struct auth_plugin));
  if (auth_plugin == NULL)
    {
      /* FIXME: propagate an ENOMEM back to the app. */
      return 0;
    }
  auth_plugin->info = info;
  auth_plugin->module = module;
  auth_plugin->next = NULL;
  if (client_plugins == NULL)
    client_plugins = auth_plugin;
  else
    end_client_plugins->next = auth_plugin;
  end_client_plugins = auth_plugin;
  return 1;
}

static const struct auth_client_plugin *
load_client_plugin (const char *name)
{
  void *module;
  char *plugin;
  const struct auth_client_plugin *info;

  assert (name != NULL);

  /* Try searching for a plugin. */
  plugin = plugin_name (name);
  if (plugin == NULL)
    return NULL;
  module = dlopen (plugin, RTLD_LAZY);
  free (plugin);
  if (module == NULL)
    return NULL;

  info = dlsym (module, "sasl_client");
  if (info == NULL || info->response == NULL)
    {
      dlclose (module);
      return NULL;
    }

  /* This plugin module is OK.  Add it to the list of loaded modules.
   */
  if (!append_plugin (module, info))
    {
      dlclose (module);
      return NULL;
    }

  return info;
}

/**
 * auth_client_init() - Initialise the auth client.
 *
 * Perform any preparation necessary for the auth client modules.  Call this
 * before any other auth client APIs.
 */
void
auth_client_init (void)
{
}

/**
 * auth_client_exit() - Destroy the auth client.
 *
 * This clears any work done by auth_client_init() or any global state that may
 * be created by the authentication modules.  Auth client APIs after this is
 * called may fail unpredictably or crash.
 */
void
auth_client_exit (void)
{
  struct auth_plugin *plugin, *next;

  /* Scan the auth_plugin array and dlclose() the modules */
#ifdef USE_PTHREADS
  pthread_mutex_lock (&plugin_mutex);
#endif
  for (plugin = client_plugins; plugin != NULL; plugin = next)
    {
      next = plugin->next;
      if (plugin->module != NULL)
	dlclose (plugin->module);
      free (plugin);
    }
  client_plugins = end_client_plugins = NULL;
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&plugin_mutex);
#endif
}

/**
 * auth_create_context() - Create an authentication context.
 *
 * Create a new authentication context.
 *
 * Return: The &typedef auth_context_t.
 */
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

/**
 * auth_destroy_context() - Destroy an authentication context.
 * @context: The authentication context.
 *
 * Destroy an authentication context, releasing any resources used.
 *
 * Return: Zero on failure, non-zero on success.
 */
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

/**
 * auth_set_mechanism_flags() - Set authentication flags.
 * @context: The authentication context.
 * @set: Flags to set.
 * @clear: Flags to clear.
 *
 * Configure authentication mechanism flags which may affect operation of the
 * authentication modules. The %AUTH_PLUGIN_EXTERNAL flag is excluded from the
 * allowable flags.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
auth_set_mechanism_flags (auth_context_t context, unsigned set, unsigned clear)
{
  API_CHECK_ARGS (context != NULL, 0);

  context->flags &= AUTH_PLUGIN_EXTERNAL | ~clear;
  context->flags |= ~AUTH_PLUGIN_EXTERNAL & set;
  return 1;
}

/**
 * auth_set_mechanism_ssf() - Set security factor.
 * @context: The authentication context.
 * @min_ssf: The minimum security factor.
 *
 * Set the minimum acceptable security factor.  The exact meaning of the
 * security factor depends on the authentication type.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
auth_set_mechanism_ssf (auth_context_t context, int min_ssf)
{
  API_CHECK_ARGS (context != NULL, 0);

  context->min_ssf = min_ssf;
  return 1;
}

/**
 * auth_set_external_id() - Set the external id.
 * @context: The authentication context.
 * @identity: Authentication identity.
 *
 * Set the authentication identity for the EXTERNAL SASL mechanism.  This call
 * also configures the built-in EXTERNAL authenticator.
 *
 * The EXTERNAL mechanism is used in conjunction with authentication which has
 * already occurred at a lower level in the network stack, such as TLS.  For
 * X.509 the identity is normally that used in the relevant certificate.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
auth_set_external_id (auth_context_t context, const char *identity)
{
  static const struct auth_client_plugin external_client =
    {
    /* Plugin information */
      "EXTERNAL",
      "SASL EXTERNAL mechanism (RFC 4422)",
    /* Plugin instance */
      NULL,
      NULL,
    /* Authentication */
      (auth_response_t) 1,
      AUTH_PLUGIN_EXTERNAL,
    /* Security Layer */
      0,
      NULL,
      NULL,
    };
  struct auth_plugin *plugin;

  API_CHECK_ARGS (context != NULL, 0);

  if (context->external_id != NULL)
    free (context->external_id);
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

/**
 * auth_client_enabled() - Check if mechanism is enabled.
 * @context: The authentication context.
 *
 * Perform various checks to ensure SASL is usable.
 *
 * Note that this does not check for loaded plugins.  This is checked when
 * negotiating a mechanism with the MTA.
 *
 * Return: Non-zero if the SASL is usable, zero otherwise.
 */
int
auth_client_enabled (auth_context_t context)
{
  if (context == NULL)
    return 0;
  if (context->interact == NULL)
    return 0;
  return 1;
}

/**
 * auth_set_mechanism() - Select authentication mechanism.
 * @context: The authentication context.
 * @name: Name of the authentication mechanism.
 *
 * Perform checks, including acceptable security levels and select
 * the authentication mechanism if successful.
 *
 * Return: Zero on failure, non-zero on success.
 */
int
auth_set_mechanism (auth_context_t context, const char *name)
{
  struct auth_plugin *plugin;
  const struct auth_client_plugin *info;

  API_CHECK_ARGS (context != NULL && name != NULL, 0);

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

  /* Load the module if not found above. */
  if (info == NULL && (info = load_client_plugin (name)) == NULL)
    {
#ifdef USE_PTHREADS
      pthread_mutex_unlock (&plugin_mutex);
#endif
      return 0;
    }

  /* Check the application's requirements regarding minimum SSF and
     whether anonymous or plain text mechanisms are explicitly allowed.
     This has to be checked here since the list of loaded plugins is
     global and the plugin may have been acceptable in another context
     but not in this one.  */
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

  context->client = info;
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&plugin_mutex);
#endif
  return 1;
}

const char *
auth_mechanism_name (auth_context_t context)
{
  API_CHECK_ARGS (context != NULL && context->client != NULL, NULL);

  return context->client->keyw;
}

const char *
auth_response (auth_context_t context, const char *challenge, int *len)
{
  API_CHECK_ARGS (context != NULL
                  && context->client != NULL
                  && len != NULL
		  && ((context->client->flags & AUTH_PLUGIN_EXTERNAL)
		       || context->interact != NULL),
		  NULL);

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
  assert (context->client->response != NULL);
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
