#ifndef _auth_plugin_h
#define _auth_plugin_h
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

/****************************************************************************
 * Client side SASL plugins.
 * This header is used only by the plugins.  Structures and constants shared
 * between the plugin and the app are in auth-client.h
 ****************************************************************************/

typedef int (*auth_init_client_t) (void *pctx);
typedef void (*auth_detsroy_client_t) (void *ctx);

/* Plugins export client info through this structure */
struct auth_client_plugin
  {
  /* Plugin information */
    const char *keyw;			/* mechanism keyword */
    const char *description;		/* description of the mechanism */
  /* Plugin instance */
    auth_init_client_t init;		/* Create plugin context */
    auth_detsroy_client_t destroy;	/* Destroy plugin context */
  /* Authentication */
    auth_response_t response;		/* request response to a challenge */
    int flags;				/* plugin information */
  /* Security Layer */
    int ssf;				/* security strength */
    auth_recode_t encode;		/* encode data for transmission */
    auth_recode_t decode;		/* decode received data */
  };

#ifndef NULL
# define NULL		((void *) 0)
#endif

#endif
