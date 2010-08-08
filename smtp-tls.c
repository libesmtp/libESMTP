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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Support for the SMTP STARTTLS verb.
 */


#ifdef USE_TLS

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

#include <ctype.h>
#include "libesmtp-private.h"
#include "siobuf.h"
#include "protocol.h"


static int tls_init;
static SSL_CTX *starttls_ctx;
static smtp_starttls_passwordcb_t ctx_password_cb;
static void *ctx_password_cb_arg;


#ifdef USE_PTHREADS
#include <pthread.h>
static pthread_mutex_t starttls_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *openssl_mutex;

static void
openssl_mutexcb (int mode, int n,
		 const char *file __attribute__ ((unused)),
		 int line __attribute__ ((unused)))
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
      if (openssl_mutex == NULL)
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

/* This stuff is crude and doesn't belong here */
/* vvvvvvvvvvv */

static const char *
get_home (void)
{
  return getenv ("HOME");
}

static char *
user_pathname (char buf[], size_t buflen, const char *tail)
{
  const char *home;

  home = get_home ();
  snprintf (buf, buflen, "%s/.authenticate/%s", home, tail);
  return buf;
}

typedef enum { FILE_PROBLEM, FILE_NOT_PRESENT, FILE_OK } ckf_t;

/* Check file exists, is a regular file and contains something */
static ckf_t
check_file (const char *file)
{
  struct stat st;

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

/* Unusually this API does not require a smtp_session_t.  The data
   it sets is global.

   N.B.  If this API is not called and OpenSSL requires a password, it
         will supply a default callback which prompts on the user's tty.
         This is likely to be undesired behaviour, so the app should
         supply a callback using this function.
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

static SSL_CTX *
starttls_create_ctx (smtp_session_t session)
{
  SSL_CTX *ctx;
  char buf[2048];
  char buf2[2048];
  char *keyfile, *cafile, *capath;
  ckf_t status;

  /* The decision not to support SSL v2 and v3 but instead to use only
     TLSv1 is deliberate.  This is in line with the intentions of RFC
     3207.  Servers typically support SSL as well as TLS because some
     versions of Netscape do not support TLS.  I am assuming that all
     currently deployed servers correctly support TLS.  */
  ctx = SSL_CTX_new (TLSv1_client_method ());

  /* Load our keys and certificates.  To avoid messing with configuration
     variables etc, use fixed paths for the certificate store.  These are
     as follows :-

     ~/.authenticate/private/smtp-starttls.pem
     				the user's certificate and private key
     ~/.authenticate/ca.pem
     				the user's trusted CA list
     ~/.authenticate/ca
     				the user's hashed CA directory

     Host specific stuff follows the same structure except that its
     below ~/.authenticate/host.name

     It probably makes sense to check that the directories and/or files
     are readable only by the user who owns them.

     This structure will certainly change.  I'm on a voyage of discovery
     here!  Eventually, this code and the SASL stuff will become a
     separate library used by libESMTP (libAUTH?).  The general idea is
     that ~/.authenticate will be used to store authentication
     information for the user (eventually there might be a
     ({/usr{/local}}/etc/authenticate for system wide stuff - CA lists
     and CRLs for example).  The "private" subdirectory is just to
     separate out private info from others.  There might be a "public"
     directory too.  Since the CA list is global (I think) I've put them
     below .authenticate for now.  Within the "private" and "public"
     directories, certificates and other authentication data are named
     according to their purpose (hence smtp-starttls.pem).  Symlinks can
     be used to avoid duplication where authentication tokens are shared
     for several purposes.  My reasoning here is that libESMTP (or any
     client layered over the hypothetical libAUTH) will always want the
     same authentication behaviour for a given service, regardless of
     the application using it.

     XXX - The above isn't quite enough.  Per-host directories are
     required, e.g. a different smtp-starttls.pem might be needed for
     different servers.  This will not affect the trusted CAs though.

     XXX - (this comment belongs in auth-client.c) Ideally, the
     ~/.authenticate hierarchy will be able to store SASL passwords
     if required, preferably encrypted.  Then the application would
     not necessarily have to supply usernames and passwords via the
     libESMTP API to be able to authenticate to a server.
   */

  /* Client certificate policy: if a client certificate is found at
     ~/.authenticate/private/smtp-starttls.pem, it is presented to the
     server if it requests it.  The server may use the certificate to
     perform authentication at its own discretion. */
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
	  return NULL;
	}
      if (!SSL_CTX_use_PrivateKey_file (ctx, keyfile, SSL_FILETYPE_PEM))
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
      return NULL;
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
      return NULL;
    }

  /* Load the CAs we trust */
  if (cafile != NULL || capath != NULL)
    SSL_CTX_load_verify_locations (ctx, cafile, capath);
  else
    SSL_CTX_set_default_verify_paths (ctx);

  /* FIXME: load a source of randomness */

  return ctx;
}

static SSL *
starttls_create_ssl (smtp_session_t session)
{
  char buf[2048];
  char buf2[2048];
  char *keyfile;
  SSL *ssl;
  ckf_t status;

  ssl = SSL_new (session->starttls_ctx);

  /* Client certificate policy: if a host specific client certificate
     is found at ~/.authenticate/host.name/private/smtp-starttls.pem,
     it is presented to the server if it requests it. */

  /* FIXME: when the default client certificate is loaded a passowrd may be
            required.  Then the code below might ask for one too.  It
            will be annoying when two passwords are needed and only one
            is necessary.  Also, the default certificate will only need
            the password when the SSL_CTX is created.  A host specific
            certificate's password will be needed for every SMTP session
            within an application.  This needs a solution.  */

  /* FIXME: in protocol.c, record the canonic name of the host returned
	    by getaddrinfo.  Use that instead of session->host. */
  snprintf (buf2, sizeof buf2, "%s/private/smtp-starttls.pem", session->host);
  keyfile = user_pathname (buf, sizeof buf, buf2);
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
  /* FIXME: if the server has reported the TLS extension in a previous
            session promote Starttls_ENABLED to Starttls_REQUIRED.
            If this session does not offer STARTTLS, this will force
            protocol.c to report the extension as not available and QUIT
            as reccommended in RFC 3207.  This requires some form of db
	    storage to record this for future sessions. */
  /* if (...)
      session->starttls_enabled = Starttls_REQUIRED; */
  if (!(session->extensions & EXT_STARTTLS))
    return 0;
  if (!session->starttls_enabled)
    return 0;
#ifdef USE_PTHREADS
  pthread_mutex_lock (&starttls_mutex);
#endif
  if (starttls_ctx == NULL && starttls_init ())
    starttls_ctx = starttls_create_ctx (session);
#ifdef USE_PTHREADS
  pthread_mutex_unlock (&starttls_mutex);
#endif
  session->starttls_ctx = starttls_ctx;
  return session->starttls_ctx != NULL;
}

static int
match_component (const char *dom, const char *edom,
                 const char *ref, const char *eref)
{
  /* If ref is the single character '*' then accept this as a wildcard
     matching any valid domainname component, i.e. characters from the
     range A-Z, a-z, 0-9, - or _ 
     NB this is more restrictive than RFC 2818 which allows multiple
     wildcard characters in the component pattern */
  if (eref == ref + 1 && *ref == '*')
    while (dom < edom)
      {
	if (!(isalnum (*dom) || *dom == '-' /*|| *dom == '_'*/))
	  return 0;
        dom++;
      }
  else
    {
      while (dom < edom && ref < eref)
	{
	  /* check for valid domainname character */
	  if (!(isalnum (*dom) || *dom == '-' /*|| *dom == '_'*/))
	    return 0;
	  /* compare the domain name case-insensitively */
	  if (!(*dom == *ref || tolower (*dom) == tolower (*ref)))
	    return 0;
	  ref++, dom++;
	}
      if (dom < edom || ref < eref)
	return 0;
    }
  return 1;
}

/* Perform a domain name comparison where the reference may contain
   wildcards.  This implements the comparison from RFC 2818.
   Each component of the domain name is matched separately, working from
   right to left.
 */
static int
match_domain (const char *domain, const char *reference)
{
  const char *dom, *edom, *ref, *eref;

  eref = strchr (reference, '\0');
  edom = strchr (domain, '\0');
  while (eref > reference && edom > domain)
    {
      /* Find the rightmost component of the reference. */
      ref = memrchr (reference, '.', eref - reference - 1);
      if (ref != NULL)
        ref++;
      else
        ref = reference;

      /* Find the rightmost component of the domain name. */
      dom = memrchr (domain, '.', edom - domain - 1);
      if (dom != NULL)
        dom++;
      else
        dom = domain;

      if (!match_component (dom, edom, ref, eref))
        return 0;
      edom = dom - 1;
      eref = ref - 1;
    }
  return eref < reference && edom < domain;
}

static int
check_acceptable_security (smtp_session_t session, SSL *ssl)
{
  X509 *cert;
  int bits;
  long vfy_result;
  int ok;

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
         get non-empty  error log in wrong places. */
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
		  if (strlen (ia5str) == ia5len
		      && match_domain (session->host, ia5str))
		    ok = 1;
		  else
		    {
		      buf[0] = '\0';
		      strncat (buf, ia5str, sizeof buf - 1);
		    }
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
		  unsigned char *str = NULL;
		  size_t len = ASN1_STRING_to_UTF8 (&str, cn);

		  if (str != NULL)
		    {
		      if (strlen ((char *) str) == len
			  && match_domain (session->host, (char *) str))
			ok = 1;
		      else
			{
			  buf[0] = '\0';
			  strncat (buf, (char *) str, sizeof buf - 1);
			}
		      OPENSSL_free (str);
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
