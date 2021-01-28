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

#include <missing.h> /* declarations for missing library functions */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#if HAVE_LWRES_NETDB_H
# include <lwres/netdb.h>
#else
# include <netdb.h>
#endif
#include "libesmtp-private.h"
#include "api.h"

/**
 * DOC: Errors
 *
 * Errors
 * ------
 *
 * Thread safe "errno" for libESMTP.
 */

struct errno_vars
  {
    int error;
    int herror;
  };

static inline void
set_error_internal (struct errno_vars *err, int code)
{
  err->error = code;
  err->herror = 0;
}

static inline void
set_herror_internal (struct errno_vars *err, int code)
{
  err->herror = code;
  if (err->herror == EAI_SYSTEM)
    err->error = errno;
}

/* Map error codes from getaddrinfo to/from those used by libESMTP. RFC
   3493 is silent on whether these values are +ve, -ve, how they sort or
   even whether they are contiguous so the mapping is done with a
   switch.  NB EAI_SYSTEM is *not* mapped. */

static int
eai_to_libesmtp (int code)
{
#define MAP(code)	case code: return SMTP_ERR_##code;
  switch (code)
    {
    MAP(EAI_AGAIN)
    MAP(EAI_FAIL)
    MAP(EAI_MEMORY)
#ifdef EAI_ADDRFAMILY	/* it seems OSX does not define this */
    MAP(EAI_ADDRFAMILY)
#endif
#ifdef EAI_NODATA	/* it seems OSX does not define this */
    MAP(EAI_NODATA)
#endif
    MAP(EAI_FAMILY)
    MAP(EAI_BADFLAGS)
    MAP(EAI_NONAME)
    MAP(EAI_SERVICE)
    MAP(EAI_SOCKTYPE)
    default: return SMTP_ERR_INVAL;
    }
#undef MAP
}

static int
libesmtp_to_eai (int code)
{
#define MAP(code)	case SMTP_ERR_##code: return code;
  switch (code)
    {
    MAP(EAI_AGAIN)
    MAP(EAI_FAIL)
    MAP(EAI_MEMORY)
#ifdef EAI_ADDRFAMILY
    MAP(EAI_ADDRFAMILY)
#endif
#ifdef EAI_NODATA
    MAP(EAI_NODATA)
#endif
    MAP(EAI_FAMILY)
    MAP(EAI_BADFLAGS)
    MAP(EAI_NONAME)
    MAP(EAI_SERVICE)
    MAP(EAI_SOCKTYPE)
    default: return 0;
    }
#undef MAP
}

static inline int
get_error_internal (struct errno_vars *err)
{
  if (err->herror == 0 || err->herror == EAI_SYSTEM)
    return err->error;
  return eai_to_libesmtp (err->herror);
}

#ifndef USE_PTHREADS

static struct errno_vars libesmtp_errno;

void
set_error (int code)
{
  set_error_internal (&libesmtp_errno, code);
}

void
set_herror (int code)
{
  set_herror_internal (&libesmtp_errno, code);
}

int
smtp_errno (void)
{
  return get_error_internal (&libesmtp_errno);
}

#else

#include <pthread.h>

static pthread_key_t libesmtp_errno;
static pthread_once_t libesmtp_errno_once = PTHREAD_ONCE_INIT;

static void
errno_destroy (void *value)
{
  if (value != NULL)
    free (value);
}

static void
errno_alloc (void)
{
  pthread_key_create (&libesmtp_errno, errno_destroy);
}

static struct errno_vars *
errno_ptr (void)
{
  struct errno_vars *value;

  pthread_once (&libesmtp_errno_once, errno_alloc);
  value = pthread_getspecific (libesmtp_errno);
  if (value == NULL)
    {
      value = malloc (sizeof (struct errno_vars));
      /* FIXME: check for NULL malloc */
      memset (value, 0, sizeof (struct errno_vars));
      pthread_setspecific (libesmtp_errno, value);
    }
  return value;
}

void
set_error (int code)
{
  struct errno_vars *value = errno_ptr ();

  if (value != NULL)
    set_error_internal (value, code);
}

void
set_herror (int code)
{
  struct errno_vars *value = errno_ptr ();

  if (value != NULL)
    set_herror_internal (value, code);
}

/**
 * smtp_errno() - Get error number.
 *
 * Retrieve the error code for the most recently failed API in the calling
 * thread.
 *
 * Return: libESMTP error code.
 */
int
smtp_errno (void)
{
  struct errno_vars *value = errno_ptr ();

  return (value != NULL) ? get_error_internal (value) : ENOMEM;
}

#endif

/* store the value of errno in libESMTP's error variable. */
void
set_errno (int code)
{
  set_error (-code);
}

static const char *libesmtp_errors[] =
  {
    "No Error",
    "",
    "Nothing to do",					/* NOTHING_TO_DO */
    "SMTP server dropped connection",			/* DROPPED_CONNECTION */
    "Invalid SMTP syntax in server response",		/* INVALID_RESPONSE_SYNTAX */
    "SMTP Status code mismatch on continuation line",	/* STATUS_MISMATCH */
    "Invalid SMTP status code in server response",	/* INVALID_RESPONSE_STATUS */
    "Invalid API function argument",			/* INVAL */
    "Requested SMTP extension not available",		/* EXTENSION_NOT_AVAILABLE */
    /* Next 10 codes handled by gai_strerror() */
    NULL,						/* EAI_ADDRFAMILY */
    NULL,						/* EAI_NODATA */
    NULL,						/* EAI_FAIL */
    NULL,						/* EAI_AGAIN */
    NULL,						/* EAI_MEMORY */
    NULL,						/* EAI_FAMILY */
    NULL,						/* EAI_BADFLAGS */
    NULL,						/* EAI_NONAME */
    NULL,						/* EAI_SERVICE */
    NULL,						/* EAI_SOCKTYPE */
    "Unterminated server response",			/* UNTERMINATED_RESPONSE */
    "Client error",					/* CLIENT_ERROR */
  };

/**
 * smtp_strerror() - Translate error number to text.
 * @error:	Error number to translate
 * @buf:	Buffer to receive text
 * @buflen:	Buffer length
 *
 * Translate a libESMTP error number to a string suitable for use in an
 * application error message.  The resuting string is copied into @buf.
 *
 * Return: A pointer to @buf on success or %NULL on failure.
 */
char *
smtp_strerror (int error, char buf[], size_t buflen)
{
  const char *text;
  int len;
  int map;

  SMTPAPI_CHECK_ARGS (buf != NULL && buflen > 0, NULL);

  if (error < 0)
#if HAVE_WORKING_STRERROR_R
    return strerror_r (-error, buf, buflen);
#elif HAVE_STRERROR_R
    {
      /* Assume the broken OSF1 strerror_r which returns an int. */
      int n = strerror_r (-error, buf, buflen);

      return n >= 0 ? buf : NULL;
    }
#else
    /* Could end up here when threading is enabled but a working
       strerror_r() is not found.  There will be a critical section
       of code until the returned string is copied to the supplied
       buffer.	This could be solved using a mutex but its hardly
       worth it since even if libESMTP is protected against itself
       the application could still call strerror anyway. */
    text = strerror (-error);
#endif
  else if ((map = libesmtp_to_eai (error)) != 0)
    text = gai_strerror (map);
  else if (error < (int) (sizeof libesmtp_errors / sizeof libesmtp_errors[0]))
    text = libesmtp_errors[error];
  else
    text = (const char *) 0;

  if (text == (const char *) 0)
    len = snprintf (buf, buflen, "Error %d", error);
  else
    {
      len = strlen (text);
      if (len > (int) buflen - 1)
	len = buflen - 1;
      if (len > 0)
	memcpy (buf, text, len);
      buf[len] = '\0';
    }
  return len >= 0 ? buf : NULL;
}
