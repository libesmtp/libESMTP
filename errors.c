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

#define _SVID_SOURCE	1	/* Need this to get strerror_r() */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "libesmtp-private.h"
#include "api.h"

#ifndef USE_PTHREADS
/* This is the totally naive and useless version, I can't believe I've
   actually just written this!  Less embarrasing code follows.
 */

static int libesmtp_errno;

/* The library's internal function */
void
set_error (int code)
{
  libesmtp_errno = code;
}

/* Internal version of the library's API function */
int
smtp_errno (void)
{
  return libesmtp_errno;
}

#else
/* Real, non-embarrasing code :-) */

#include <pthread.h>

struct errno_vars
  {
    int error;
  };

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
    value->error = code;
}

int
smtp_errno (void)
{
  struct errno_vars *value = errno_ptr ();

  return (value != NULL) ? value->error : 0;
}

#endif

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
    "Host not found",					/* HOST_NOT_FOUND */
    "No address",					/* NO_ADDRESS */
    "No recovery",					/* NO_RECOVERY */
    "Temporary DNS failure; try again later",		/* TRY_AGAIN */
  };

char *
smtp_strerror (int error, char buf[], size_t buflen)
{
  const char *text;
  size_t len;

  SMTPAPI_CHECK_ARGS (buf != NULL && buflen > 0, NULL);

  if (error < 0)
#ifdef HAVE_WORKING_STRERROR_R
    return strerror_r (-error, buf, buflen);
#else
    /* Could end up here when threading is enabled but a working
       strerror_r() is not found.  There will be a critical section
       of code until the returned string is copied to the supplied
       buffer.  This could be solved using a mutex but its hardly
       worth it since even if libESMTP is protected against itself
       the application could still call strerror anyway. */
    text = strerror (-error);
#endif
  else if (error < (int) (sizeof libesmtp_errors / sizeof libesmtp_errors[0]))
    text = libesmtp_errors[error];
  else
    text = (const char *) 0;

  if (text == (const char *) 0)
    snprintf (buf, buflen, "Error %d", error);
  else
    {
      len = strlen (text);
      if (len > buflen - 1)
        len = buflen - 1;
      memcpy (buf, text, len);
      buf[len] = '\0';
    }
  return buf;
}

/* Map the values of h_errno to libESMTP's error codes */
void
set_herror (int code)
{
  int smtp_code;

#ifdef HAVE_GETADDRINFO
  /* Very crude mapping of the error codes on to existing ones.
   */
  switch (code)
    {
    case EAI_AGAIN:       /* temporary failure in name resolution */
      smtp_code = SMTP_ERR_TRY_AGAIN;
      break;
    case EAI_FAIL:        /* non-recoverable failure in name resolution */
      smtp_code = SMTP_ERR_NO_RECOVERY;
      break;
    case EAI_MEMORY:      /* memory allocation failure */
      set_error (-ENOMEM);
      return;
    case EAI_ADDRFAMILY:  /* address family for nodename not supported */
      smtp_code = SMTP_ERR_HOST_NOT_FOUND;
      break;
    case EAI_NODATA:      /* no address associated with nodename */
      smtp_code = SMTP_ERR_NO_ADDRESS;
      break;
    case EAI_SYSTEM:      /* system error returned in errno */
      set_error (-errno);
      return;
    case EAI_FAMILY:      /* ai_family not supported */
    case EAI_BADFLAGS:    /* invalid value for ai_flags */
    case EAI_NONAME:      /* nodename nor servname provided, or not known */
    case EAI_SERVICE:     /* servname not supported for ai_socktype */
    case EAI_SOCKTYPE:    /* ai_socktype not supported */
    default: /* desperation */
      smtp_code = SMTP_ERR_INVAL;
      break;
    }
#else
  switch (code)
    {
    case HOST_NOT_FOUND:	smtp_code = SMTP_ERR_HOST_NOT_FOUND; break;
    case NO_ADDRESS:		smtp_code = SMTP_ERR_NO_ADDRESS; break;
    case NO_RECOVERY:		smtp_code = SMTP_ERR_NO_RECOVERY; break;
    case TRY_AGAIN:		smtp_code = SMTP_ERR_TRY_AGAIN; break;
    default: /* desperation */	smtp_code = SMTP_ERR_INVAL; break;
    }
#endif
  set_error (smtp_code);
}

/* store the value of errno in libESMTP's error variable. */
void
set_errno (int code)
{
  set_error (-code);
}
