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

/* Support for the SMTP STARTTLS verb.
 */


#ifdef USE_TLS

#include "libesmtp-private.h"
#include "siobuf.h"
#include "protocol.h"

static int tls_init;
static SSL_CTX *starttls_ctx;

#ifdef USE_PTHREADS
#include <pthread.h>
static pthread_mutex_t starttls_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *openssl_mutex;

static void
openssl_mutexcb (int mode, int n,
		 const char *file __attribute ((unused)),
		 int line __attribute ((unused)))
{
  if (mode & CRYPTO_LOCK)
    pthread_mutex_lock (&openssl_mutex[n]);
  else
    pthread_mutex_unlock (&openssl_mutex[n]);
}
#endif

static int
starttls_init (void)
{
  if (tls_init)
    return 1;

#ifdef USE_PTHREADS
  /* Set up mutexes for the OpenSSL library */
  if (openssl_mutex == NULL)
    {
      pthread_mutexattr_t attr;
      int n;

      openssl_mutex = malloc (sizeof (pthread_mutex_t) * CRYPTO_num_locks ());
      if (openssl_mutex != NULL)
        return 0;
      pthread_mutexattr_init (&attr);
      for (n = 0; n < CRYPTO_num_locks (); n++)
	pthread_mutex_init (&openssl_mutex[n], &attr);
      pthread_mutexattr_destroy (&attr);
      CRYPTO_set_locking_callback (openssl_mutexcb);
    }
#endif
  tls_init = 1;
  SSL_load_error_strings ();
  SSL_library_init ();
  return 1;
}

/* App calls this to allow libESMTP to use an SSL_CTX it has already
   initialised.  NULL means use a default created by libESMTP.
   If called at all, libESMTP assumes the application has initialised
   openssl.  Otherwise, libESMTP will initialise OpenSSL before calling
   any of the SSL APIs. */
int
smtp_starttls_set_ctx (smtp_session_t session, SSL_CTX *ctx)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  tls_init = 1;			/* Assume app has set up openssl */
  session->starttls_ctx = ctx;
  return 1;
}

/* how == 0: disabled, 1: if possible, 2: required */
int
smtp_starttls_enable (smtp_session_t session, enum starttls_option how)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  session->starttls_enabled = how;
  if (how == Starttls_REQUIRED)
    session->required_extensions |= EXT_STARTTLS;
  else
    session->required_extensions &= ~EXT_STARTTLS;
  return 1;
}

int
select_starttls (smtp_session_t session)
{
  if (session->using_tls || session->authenticated)
    return 0;
  if (!session->starttls_enabled)
    return 0;
#ifdef USE_PTHREADS
  pthread_mutex_lock (&starttls_mutex);
#endif
  if (starttls_ctx == NULL && starttls_init ())
    starttls_ctx = SSL_CTX_new (SSLv23_method ());
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&starttls_mutex);
#endif
  session->starttls_ctx = starttls_ctx;
  return session->starttls_ctx != NULL;
}

void
cmd_starttls (siobuf_t conn, smtp_session_t session)
{
  sio_write (conn, "STARTTLS\r\n", -1);
  session->cmd_state = -1;
}

void
rsp_starttls (siobuf_t conn, smtp_session_t session)
{
  int code;

  code = read_smtp_response (conn, session, &session->mta_status, NULL);
  if (code == 2
      && sio_set_tlsclient_ctx (conn, session->starttls_ctx))
    {
      /* TODO: use the event callback report interesting stuff such as
               the cipher in use, server cert info etc. */

      /* Forget what we know about the server and reset protocol state.
       */
      session->extensions = 0;
      destroy_auth_mechanisms (session);
#if 0     /* TODO */
      session->rsp_state = check_acceptable_security_level () ? S_ehlo
      							      : S_quit; */
#else
      session->rsp_state = S_ehlo;
#endif
    }
  else
    {
      /* If the application does not require the use of TLS move on
	 to the mail command since the a publicly referenced MTA is
	 required to accept mail for its own domain. */
      if (session->starttls_enabled == 2)
	session->rsp_state = S_quit;
#ifdef USE_ETRN
      else if (check_etrn (session))
	session->rsp_state = S_etrn;
#endif
      else
	session->rsp_state = initial_transaction_state (session);
    }
}

#else

#define SSL_CTX void
#include "libesmtp-private.h"

/* Fudge the declaration.  The idea is that all builds of the library
   export the same API, but that the unsupported features always fail.
   This prototype is declared only if <openssl/ssl.h> is included.  */
int smtp_starttls_set_ctx (smtp_session_t session, SSL_CTX *ctx);

int
smtp_starttls_set_ctx (smtp_session_t session, SSL_CTX *ctx)
{
  SMTPAPI_CHECK_ARGS (session != (smtp_session_t) 0, 0);

  return 0;
}

int
smtp_starttls_enable (smtp_session_t session, enum starttls_option how)
{
  SMTPAPI_CHECK_ARGS (session != (smtp_session_t) 0, 0);

  return 0;
}


#endif
