#ifndef _auth_client_h
#define _auth_client_h
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

#ifdef __cplusplus
extern "C" {
#endif

/* Someday this will expand into a standalone SASL library.  For now the
   application must use the remainder of the SASL implementation's API
   to set things up before calling smtp_start_session(). */

typedef struct auth_context *auth_context_t;

/* These flags are set for standard user authentication info */
#define AUTH_USER			0x0001
#define AUTH_REALM			0x0002
#define AUTH_PASS			0x0004

/* This flag is set for information passed in clear text on the wire */
#define AUTH_CLEARTEXT			0x0008

struct auth_client_request
  {
    const char *name;	/* Name of field requested from the application,
			   e.g. "user", "passphrase" "realm" etc. */
    unsigned flags;	/* Alternative version of above */
    const char *prompt;	/* Text that the application can use to prompt
                           for input. */
    unsigned size;	/* Maximum length of response allowed. 0 == no limit */
  };
typedef const struct auth_client_request *auth_client_request_t;
typedef int (*auth_interact_t) (auth_client_request_t request,
				char **result, int fields, void *arg);
typedef const char *(*auth_response_t) (void *ctx,
				        const char *challenge, int *len,
				        auth_interact_t interact, void *arg);

typedef int (*auth_recode_t) (void *ctx, char **dstbuf, int *dstlen,
	     		      const char *srcbuf, int srclen);

/* For enabling mechanisms */
#define AUTH_PLUGIN_ANONYMOUS	0x01	/* mechanism is anonymous */
#define AUTH_PLUGIN_PLAIN	0x02	/* mechanism uses plaintext passwords */
#define AUTH_PLUGIN_EXTERNAL	0x04	/* mechanism is EXTERNAL */


void auth_client_init(void);
void auth_client_exit(void);
auth_context_t auth_create_context(void);
int auth_destroy_context(auth_context_t context);
int auth_set_mechanism_flags (auth_context_t context,
	                      unsigned set, unsigned clear);
int auth_set_mechanism_ssf (auth_context_t context, int min_ssf);
int auth_set_interact_cb (auth_context_t context,
			  auth_interact_t interact, void *arg);
int auth_client_enabled(auth_context_t context);
int auth_set_mechanism(auth_context_t context, const char *name);
const char *auth_mechanism_name (auth_context_t context);
const char *auth_response(auth_context_t context, const char *challenge, int *len);
int auth_get_ssf(auth_context_t context);
void auth_encode(char **dstbuf, int *dstlen, const char *srcbuf, int srclen, void *arg);
void auth_decode(char **dstbuf, int *dstlen, const char *srcbuf, int srclen, void *arg);
int auth_set_external_id (auth_context_t context, const char *identity);

#ifdef __cplusplus
};
#endif


#endif
