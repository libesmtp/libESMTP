/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2001-2004  Brian Stafford  <brian@stafford.uklinux.net>
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

/* Support for the SMTP STARTTLS verb.
 */


#ifdef USE_TLS

/**
 * DOC: RFC 2487
 *
 * StartTLS Extension
 * ------------------
 *
 * If OpenSSL is available when building libESMTP, support for the STARTTLS
 * extension can be enabled.  If support is not enabled, the following APIs
 * will always fail:
 *
 * * smtp_starttls_set_password_cb()
 * * smtp_starttls_set_ctx()
 * * smtp_starttls_enable()
 *
 * See also: `OpenSSL <https://www.openssl.org/>`_.
 */

/* This stuff doesn't belong here */
/* vvvvvvvvvvv */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <missing.h> /* declarations for missing library functions */
/* ^^^^^^^^^^^ */

#include "libesmtp-private.h"
#include "siobuf.h"
#include "protocol.h"
#include "attribute.h"

#include "tlsutils.h"

static smtp_starttls_passwordcb_t ctx_password_cb;
static void *ctx_password_cb_arg;


#ifdef USE_PTHREADS
#include <pthread.h>
static pthread_mutex_t starttls_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* This stuff is crude and doesn't belong here */
/* vvvvvvvvvvv */

static const char *
get_home (void)
{
  return getenv ("HOME");
}

#ifdef USE_XDG_DIRS
# define XDG	".config"
# define APP	"libesmtp"

static const char *
get_config_dir (void)
{
  return getenv ("XDG_CONFIG_DIR");
}

static char *
user_pathname (char buf[], size_t buflen, const char *tail)
{
  const char *home;
  int len;

  home = get_config_dir ();
  if (home != NULL)
    len = snprintf (buf, buflen, "%s/"APP"/%s", home, tail);
  else
    {
      home = get_home ();
      len = snprintf (buf, buflen, "%s/"XDG"/"APP"/%s", home, tail);
    }
  return (len >= 0 && (size_t) len < buflen) ? buf : NULL;
}

static char *
host_pathname (char buf[], size_t buflen, smtp_session_t session,
	       const char *dir, const char *suffix)
{
  const char *home, *host;
  int len;

  /* use the canonic hostname if available */
  host = session->canon != NULL ? session->canon : session->host;

  home = get_config_dir ();
  if (home != NULL)
    len = snprintf (buf, buflen, "%s/"APP"/%s/%s.%s", home, dir, host, suffix);
  else
    {
      home = get_home ();
      len = snprintf (buf, buflen, "%s/"XDG"/"APP"/%s/%s.%s",
		      home, dir, host, suffix);
    }
  return (len >= 0 && (size_t) len < buflen) ? buf : NULL;
}

#else

static char *
user_pathname (char buf[], size_t buflen, const char *tail)
{
  const char *home;
  int len;

  home = get_home ();
  len = snprintf (buf, buflen, "%s/.authenticate/%s", home, tail);
  return (len >= 0 && (size_t) len < buflen) ? buf : NULL;
}

static char *
host_pathname (char buf[], size_t buflen, smtp_session_t session,
	       const char *tail)
{
  const char *home, *host;
  int len;

  home = get_home ();
  /* use the canonic hostname if available */
  host = session->canon != NULL ? session->canon : session->host;
  len = snprintf (buf, buflen, "%s/.authenticate/%s/%s", home, host, tail);
  return (len >= 0 && (size_t) len < buflen) ? buf : NULL;
}

#endif

typedef enum { FILE_PROBLEM, FILE_NOT_PRESENT, FILE_OK } ckf_t;

/* Check file exists, is a regular file and contains something */
static ckf_t
check_file (const char *file)
{
  struct stat st;

  if (file == NULL)
    return FILE_PROBLEM;

  errno = 0;
  if (stat (file, &st) < 0)
    return (errno == ENOENT) ? FILE_NOT_PRESENT : FILE_PROBLEM;
  /* File must be regular and contain something */
  if (!(S_ISREG (st.st_mode) && st.st_size > 0))
    return FILE_PROBLEM;
  /* For now this check is paranoid.  The way I figure it, the passwords
     on the private keys will be intensely annoying, so people will
     remove them.  Therefore make them protect the files using very
     restrictive file permissions.  Only the owner should be able to
     read or write the certificates and the user the app is running as
     should own the files. */
  if ((st.st_mode & (S_IXUSR | S_IRWXG | S_IRWXO)) || st.st_uid != getuid ())
    return FILE_PROBLEM;
  return FILE_OK;
}

/* Check directory exists */
static ckf_t
check_directory (const char *file)
{
  struct stat st;

  if (file == NULL)
    return FILE_PROBLEM;

  if (stat (file, &st) < 0)
    return (errno == ENOENT) ? FILE_NOT_PRESENT : FILE_PROBLEM;
  /* File must be a directory */
  if (!S_ISDIR (st.st_mode))
    return FILE_PROBLEM;
  /* Paranoia - only permit owner rwx permissions */
  if ((st.st_mode & (S_IRWXG | S_IRWXO)) || st.st_uid != getuid ())
    return FILE_PROBLEM;
  return FILE_OK;
}

/* ^^^^^^^^^^^ */

/**
 * smtp_starttls_set_password_cb() - Set OpenSSL password callback.
 * @cb: Password callback with signature &smtp_starttls_passwordcb_t.
 * @arg: User data passed to the callback.
 *
 * Set password callback function for OpenSSL. Unusually this API does not
 * require a &typedef smtp_session_t as the data it sets is global.
 *
 * N.B.  If this API is not called and OpenSSL requires a password, it will
 * supply a default callback which prompts on the user's tty.  This is likely
 * to be undesired behaviour, so the app should supply a callback using this
 * function.
 *
 * Returns: Zero on failure, non-zero on success.
 */
int
smtp_starttls_set_password_cb (smtp_starttls_passwordcb_t cb, void *arg)
{
  SMTPAPI_CHECK_ARGS (cb != NULL, 0);

#ifdef USE_PTHREADS
  pthread_mutex_lock (&starttls_mutex);
#endif
  ctx_password_cb = cb;
  ctx_password_cb_arg = arg;
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&starttls_mutex);
#endif
  return 1;
}

static int
starttls_init_ctx (smtp_session_t session, SSL_CTX *ctx)
{
  char buf[2048];
  char buf2[2016];
  char *keyfile, *cafile, *capath;
  ckf_t status;

  /* Load our keys and certificates.  To avoid messing with configuration
     variables etc, use fixed paths for the certificate store.	These are
     as follows :-

     ~/.authenticate/private/smtp-starttls.pem
				the user's certificate and private key
     ~/.authenticate/ca.pem
				the user's trusted CA list
     ~/.authenticate/ca
				the user's hashed CA directory

     Host specific stuff follows the same structure except that it is
     below ~/.authenticate/host.name

     Files and directories must be readable only by their owner.
     User certificates are presented to the server only on request.
   */
  if (ctx_password_cb != NULL)
    {
      SSL_CTX_set_default_passwd_cb (ctx, ctx_password_cb);
      SSL_CTX_set_default_passwd_cb_userdata (ctx, ctx_password_cb_arg);
    }
  keyfile = user_pathname (buf, sizeof buf, "private/smtp-starttls.pem");
  status = check_file (keyfile);
  if (status == FILE_OK)
    {
      if (!SSL_CTX_use_certificate_file (ctx, keyfile, SSL_FILETYPE_PEM))
	{
	  /* FIXME: set an error code */
	  return 0;
	}
      if (!SSL_CTX_use_PrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM))
	{
	  int ok = 0;
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_NO_CLIENT_CERTIFICATE,
				  session->event_cb_arg, &ok);
	  if (!ok)
	    return 0;
	}
    }
  else if (status == FILE_PROBLEM)
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_UNUSABLE_CLIENT_CERTIFICATE,
			      session->event_cb_arg, NULL);
      return 0;
    }

  /* Server certificate policy: check the server certificate against the
     trusted CA list to a depth of 1. */
  cafile = user_pathname (buf, sizeof buf, "ca.pem");
  status = check_file (cafile);
  if (status == FILE_NOT_PRESENT)
    cafile = NULL;
  else if (status == FILE_PROBLEM)
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_UNUSABLE_CA_LIST,
			      session->event_cb_arg, NULL);
      return 0;
    }
  capath = user_pathname (buf2, sizeof buf2, "ca");
  status = check_directory (capath);
  if (status == FILE_NOT_PRESENT)
    capath = NULL;
  else if (status == FILE_PROBLEM)
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_UNUSABLE_CA_LIST,
			      session->event_cb_arg, NULL);
      return 0;
    }

  /* Load the CAs we trust */
  if (cafile != NULL || capath != NULL)
    SSL_CTX_load_verify_locations (ctx, cafile, capath);
  else
    SSL_CTX_set_default_verify_paths (ctx);

  return 1;
}

static SSL_CTX *
starttls_create_ctx (smtp_session_t session)
{
  SSL_CTX *ctx;
  static SSL_CTX *starttls_ctx;

#ifdef USE_PTHREADS
  pthread_mutex_lock (&starttls_mutex);
#endif
  if (starttls_ctx != NULL)
    ctx = starttls_ctx;
  else
    {
      /* The decision not to support SSL v2 and v3 but instead to use only
	 TLSv1.X is deliberate.  This is in line with the intentions of RFC
	 3207.  Servers typically support SSL as well as TLS because some
	 versions of Netscape do not support TLS.  I am assuming that all
	 currently deployed servers correctly support TLS.	*/
      ctx = SSL_CTX_new (TLS_client_method ());
      if (ctx != NULL)
	{
	  SSL_CTX_set_min_proto_version (ctx, TLS1_VERSION);

	  if (!starttls_init_ctx (session, ctx))
	    {
	      SSL_CTX_free (ctx);
	      ctx = NULL;
	    }
	}
      starttls_ctx = ctx;
    }
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&starttls_mutex);
#endif
  return ctx;
}

void
destroy_starttls_context (smtp_session_t session)
{
  SSL_CTX_free (session->starttls_ctx);
}

static SSL *
starttls_create_ssl (smtp_session_t session)
{
  char buf[2048];
  char *keyfile;
  SSL *ssl;
  ckf_t status;

  ssl = SSL_new (session->starttls_ctx);

  /* Client certificate policy: if a host specific client certificate
     is found it is presented to the server if requested. */

#ifdef USE_XDG_DIRS
  /* check for client certificate file:
     ~/<xdg-config>/libesmtp/private/<host>.pem */
  keyfile = host_pathname (buf, sizeof buf, session, "private", "pem");
#else
  /* check for client certificate file:
     ~/.authenticate/<host>/private/smtp-starttls.pem */
  keyfile = host_pathname (buf, sizeof buf, session,
  			   "private/smtp-starttls.pem");
#endif
  status = check_file (keyfile);
  if (status == FILE_OK)
    {
      if (!SSL_use_certificate_file (ssl, keyfile, SSL_FILETYPE_PEM))
	{
	  /* FIXME: set an error code */
	  return NULL;
	}
      if (!SSL_use_PrivateKey_file (ssl, keyfile, SSL_FILETYPE_PEM))
	{
	  int ok = 0;
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_NO_CLIENT_CERTIFICATE,
				  session->event_cb_arg, &ok);
	  if (!ok)
	    return NULL;
	}
    }
  else if (status == FILE_PROBLEM)
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_UNUSABLE_CLIENT_CERTIFICATE,
			      session->event_cb_arg, NULL);
      return NULL;
    }

  return ssl;
}

/**
 * smtp_starttls_set_ctx() - Set the SSL_CTX for the SMTP session.
 * @session: The session.
 * @ctx: An SSL_CTX initialised by the application.
 *
 * Use an SSL_CTX created and initialised by the application.  The SSL_CTX
 * must be created by the application which is assumed to have also initialised
 * the OpenSSL library.
 *
 * If not used, or @ctx is %NULL, OpenSSL is automatically initialised before
 * calling any of the OpenSSL API functions.
 *
 * Returns: Zero on failure, non-zero on success.
 */
int
smtp_starttls_set_ctx (smtp_session_t session, SSL_CTX *ctx)
{
  SMTPAPI_CHECK_ARGS (session != NULL, 0);

  SSL_CTX_up_ref (ctx);
  session->starttls_ctx = ctx;
  return 1;
}

/**
 * smtp_starttls_enable() - Enable STARTTLS verb.
 * @session: The session.
 * @how: A &enum starttls_option
 *
 * Enable the SMTP STARTTLS verb if @how is not %Starttls_DISABLED.  If set to
 * %Starttls_REQUIRED the protocol will quit rather than transferring any
 * messages if the STARTTLS extension is not available.
 *
 * Returns: Zero on failure, non-zero on success.
 */
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
  /* FIXME: if the server has reported the TLS extension in a previous
	    session promote Starttls_ENABLED to Starttls_REQUIRED.
	    If this session does not offer STARTTLS, this will force
	    protocol.c to report the extension as not available and QUIT
	    as recommended in RFC 3207.  This requires some form of db
	    storage to record this for future sessions. */
  /* if (...)
      session->starttls_enabled = Starttls_REQUIRED; */
  if (!(session->extensions & EXT_STARTTLS))
    return 0;
  if (session->starttls_enabled == Starttls_DISABLED)
    return 0;
  /* Create a CTX if the application has not already done so. */
  if (session->starttls_ctx == NULL)
    session->starttls_ctx = starttls_create_ctx (session);
  return session->starttls_ctx != NULL;
}

static int
check_acceptable_security (smtp_session_t session, SSL *ssl)
{
  X509 *cert;
  const char *host;
  int bits;
  long vfy_result;
  int ok;

  /* use canonic hostname for validation if available */
  host = session->canon != NULL ? session->canon : session->host;

  /* Check certificate validity.
   */
  vfy_result = SSL_get_verify_result (ssl);
  if (vfy_result != X509_V_OK)
    {
      ok = 0;
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_INVALID_PEER_CERTIFICATE,
			      session->event_cb_arg, vfy_result, &ok, ssl);
      if (!ok)
	return 0;
#if 0
      /* Not sure about the location of this call so leave it out for now
	 - from Pawel: the worst thing that can happen is that one can
	 get non-empty	error log in wrong places. */
      ERR_clear_error ();	/* we know what is going on, clear the error log */
#endif
    }

  /* Check cipher strength.  Since we use only TLSv1 the cipher should
     never be this weak.  */
  bits = SSL_get_cipher_bits (ssl, NULL);
  if (bits <= 40)
    {
      ok = 0;
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_WEAK_CIPHER,
			      session->event_cb_arg, bits, &ok);
      if (!ok)
	return 0;
    }

  /* Check server credentials stored in the certificate.
   */
  ok = 0;
  cert = SSL_get_peer_certificate (ssl);
  if (cert == NULL)
    {
      if (session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_NO_PEER_CERTIFICATE,
			      session->event_cb_arg, &ok);
    }
  else
    {
      char buf[256] = { 0 };
      STACK_OF(GENERAL_NAME) *altnames;
      int hasaltname = 0;

      altnames = X509_get_ext_d2i (cert, NID_subject_alt_name, NULL, NULL);
      if (altnames != NULL)
	{
	  int i;

	  for (i = 0; i < sk_GENERAL_NAME_num (altnames); ++i)
	    {
	      GENERAL_NAME *name = sk_GENERAL_NAME_value (altnames, i);

	      if (name->type == GEN_DNS)
		{
		  const char *ia5str = (const char *) name->d.ia5->data;
		  size_t ia5len = name->d.ia5->length;

		  hasaltname = 1;
		  if (strlen (ia5str) == ia5len && match_domain (host, ia5str))
		    ok = 1;
		  else
		    strlcpy (buf, ia5str, sizeof buf);
		}
	      // TODO: handle GEN_IPADD
	    }
	  sk_GENERAL_NAME_pop_free (altnames, GENERAL_NAME_free);
	}

      if (!hasaltname)
	{
	  X509_NAME *subj = X509_get_subject_name (cert);

	  if (subj != NULL)
	    {
	      ASN1_STRING *cn;
	      int idx, i = -1;

	      do
		{
		  idx = i;
		  i = X509_NAME_get_index_by_NID (subj, NID_commonName, i);
		}
	      while (i >= 0);

	      if (idx >= 0
		  && (cn = X509_NAME_ENTRY_get_data (
						X509_NAME_get_entry (subj, idx)
						     )) != NULL)
		{
		  unsigned char *ustr = NULL;
		  const char *str;
		  size_t len;

		  len = ASN1_STRING_to_UTF8 (&ustr, cn);
		  if ((str = (const char *) ustr) != NULL)
		    {
		      if (strlen (str) == len && match_domain (host, str))
			ok = 1;
		      else
			strlcpy (buf, str, sizeof buf);
		      OPENSSL_free (ustr);
		    }
		}
	    }
	}

      if (!ok && session->event_cb != NULL)
	(*session->event_cb) (session, SMTP_EV_WRONG_PEER_CERTIFICATE,
			      session->event_cb_arg, &ok, buf, ssl);

      X509_free (cert);
    }
  return ok;
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
  SSL *ssl;
  X509 *cert;
  char buf[256];

  code = read_smtp_response (conn, session, &session->mta_status, NULL);
  if (code < 0)
    {
      session->rsp_state = S_quit;
      return;
    }

  if (code != 2)
    {
      if (code != 4 && code != 5)
	set_error (SMTP_ERR_INVALID_RESPONSE_STATUS);
      session->rsp_state = S_quit;
    }
  else if (sio_set_tlsclient_ssl (conn, (ssl = starttls_create_ssl (session))))
    {
      session->using_tls = 1;

      /* Forget what we know about the server and reset protocol state.
       */
      session->extensions = 0;
      destroy_auth_mechanisms (session);

      if (!check_acceptable_security (session, ssl))
	session->rsp_state = S_quit;
      else
	{
	  if (session->event_cb != NULL)
	    (*session->event_cb) (session, SMTP_EV_STARTTLS_OK,
				  session->event_cb_arg,
				  ssl, SSL_get_cipher (ssl),
				  SSL_get_cipher_bits (ssl, NULL));
	  cert = SSL_get_certificate (ssl);
	  if (cert != NULL)
	    {
	      /* Copy the common name [typically email address] from the
		 client certificate and use it to prime the SASL EXTERNAL
		 mechanism */
	      X509_NAME_get_text_by_NID (X509_get_subject_name (cert),
					 NID_commonName, buf, sizeof buf);
	      X509_free (cert);
	      if (session->auth_context != NULL)
		auth_set_external_id (session->auth_context, buf);
	    }

	  /* Next state is EHLO */
	  session->rsp_state = S_ehlo;
	}
    }
  else
    {
      set_error (SMTP_ERR_CLIENT_ERROR);
      session->rsp_state = -1;
    }
}

#else

#define SSL_CTX void
#include "libesmtp-private.h"

/* Fudge the declaration.  The idea is that all builds of the library
   export the same API, but that the unsupported features always fail.
   This prototype is declared only if <openssl/ssl.h> is included.
   Strict ANSI compiles require prototypes, so here it is! */
int smtp_starttls_set_ctx (smtp_session_t session, SSL_CTX *ctx);

int
smtp_starttls_set_ctx (smtp_session_t session,
		       SSL_CTX *ctx __attribute__ ((unused)))
{
  SMTPAPI_CHECK_ARGS (session != (smtp_session_t) 0, 0);

  return 0;
}

int
smtp_starttls_enable (smtp_session_t session,
		      enum starttls_option how __attribute__ ((unused)))
{
  SMTPAPI_CHECK_ARGS (session != (smtp_session_t) 0, 0);

  return 0;
}

int
smtp_starttls_set_password_cb (smtp_starttls_passwordcb_t cb
							__attribute__ ((unused)),
			       void *arg __attribute__ ((unused)))
{
  return 0;
}

#endif
