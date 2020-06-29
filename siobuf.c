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

#include <assert.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include <missing.h> /* declarations for missing library functions */

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>

#ifdef USE_TLS
# include <openssl/ssl.h>
#endif

#include "siobuf.h"

#ifdef USE_TLS
static int sio_sslpoll (struct siobuf *sio, int ret);
#endif

/* Socket I/O buffering */
struct siobuf
  {
    int sdr;			/* Socket descriptor being buffered. */
    int sdw;			/* Socket descriptor being buffered. */

    size_t buffer_size;		/* size of buffers */
    int milliseconds;		/* Timeout in ms */

    char *read_buffer;		/* client read buffer */
    char *read_position;	/* client read buffer pointer */
    int read_unread;		/* number of bytes unread in buffer */

    char *write_buffer;		/* client write buffer */
    char *write_position;	/* client write buffer pointer */
    char *flush_mark;		/* don't flush beyond this point */
    int write_available;	/* number of bytes available in buffer */

    monitorcb_t monitor_cb;
    void *cbarg;

    recodecb_t encode_cb;	/* encoder for outbound data */
    recodecb_t decode_cb;	/* decoder for inbound data */
    void *secarg;

#ifdef USE_TLS
    SSL *ssl;			/* The SSL connection */
#endif

    void *user_data;
  };

/* Attach bi-directional buffering to the socket descriptor.
 */
struct siobuf *
sio_attach (int sdr, int sdw, int buffer_size)
{
  struct siobuf *sio;

  sio = malloc (sizeof (struct siobuf));
  if (sio == NULL)
    return NULL;
  memset (sio, 0, sizeof (struct siobuf));
  sio->sdr = sdr;
  sio->sdw = sdw;

  /* Use non blocking io and polling to avoid the potential deadlock
     PIPELINING situation described in RFC 2920. */
  fcntl (sio->sdw, F_SETFL, O_NONBLOCK);
  if (sio->sdr != sio->sdw)
    fcntl (sio->sdr, F_SETFL, O_NONBLOCK);

  /* Allocate the buffer for reading. */
  sio->buffer_size = buffer_size;
  sio->read_position = sio->read_buffer = malloc (sio->buffer_size);
  sio->read_unread = 0;
  if (sio->read_buffer == NULL)
    {
      free (sio);
      return NULL;
    }

  /* Allocate the buffer for writing. */
  sio->write_position = sio->write_buffer = malloc (sio->buffer_size);
  if (sio->write_buffer == NULL)
    {
      free (sio->read_buffer);
      free (sio);
      return NULL;
    }
  sio->write_available = sio->buffer_size;

  sio->milliseconds = -1;
  return sio;
}

/* Detach buffering from the socket descriptor.  The socket is not closed.
 */
void
sio_detach (struct siobuf *sio)
{
  assert (sio != NULL);

#ifdef USE_TLS
  if (sio->ssl != NULL)
    {
      int ret;

      /* Send a close notify to the peer for a graceful shutdown.
       */
      while ((ret = SSL_shutdown (sio->ssl)) == 0)
        if (sio_sslpoll (sio, ret) <= 0)
	  break;
      SSL_free (sio->ssl);
    }
#endif
  free (sio->read_buffer);
  free (sio->write_buffer);
  free (sio);
}

void
sio_set_monitorcb (struct siobuf *sio, monitorcb_t cb, void *arg)
{
  assert (sio != NULL);

  sio->monitor_cb = cb;
  sio->cbarg = arg;
}

void
sio_set_timeout (struct siobuf *sio, int milliseconds)
{
  assert (sio != NULL);

  sio->milliseconds = milliseconds;
#ifdef USE_TLS
  if (sio->ssl != NULL)
    {
      long ssl_timeout;

      if (milliseconds < 0)
        ssl_timeout = 86400L;
      else
        ssl_timeout = ((long) milliseconds + 999L) / 1000L;
      SSL_SESSION_set_timeout (SSL_get_session (sio->ssl), ssl_timeout);
    }
#endif
}

#ifdef USE_TLS
int
sio_set_tlsclient_ssl (struct siobuf *sio, SSL *ssl)
{
  int ret;

  assert (sio != NULL);

  if (ssl != NULL)
    {
      sio->ssl = ssl;
      SSL_set_rfd (sio->ssl, sio->sdr);
      SSL_set_wfd (sio->ssl, sio->sdw);
      while ((ret = SSL_connect (sio->ssl)) <= 0)
        if (sio_sslpoll (sio, ret) <= 0)
	  {
	    SSL_free (sio->ssl);
	    sio->ssl = NULL;
	    break;
	  }
      sio_set_timeout (sio, sio->milliseconds);
    }
  return sio->ssl != NULL;
}

int
sio_set_tlsserver_ssl (struct siobuf *sio, SSL *ssl)
{
  int ret;

  assert (sio != NULL);

  if (ssl != NULL)
    {
      sio->ssl = ssl;
      SSL_set_rfd (sio->ssl, sio->sdr);
      SSL_set_wfd (sio->ssl, sio->sdw);
      while ((ret = SSL_accept (sio->ssl)) <= 0)
        if (sio_sslpoll (sio, ret) <= 0)
	  {
	    SSL_free (sio->ssl);
	    sio->ssl = NULL;
	    break;
	  }
      sio_set_timeout (sio, sio->milliseconds);
    }
  return sio->ssl != NULL;
}
#endif

void
sio_set_securitycb (struct siobuf *sio,
                    recodecb_t encode_cb, recodecb_t decode_cb, void *arg)
{
  assert (sio != NULL);

  sio->secarg = arg;
  sio->encode_cb = encode_cb;
  sio->decode_cb = decode_cb;
}

/* Return -1 on timeout or error.  Return 0 if nothing to poll.
   Return OR of SIO_READ, SIO_WRITE as appropriate for request.
   If the fast flag is set, poll does not block, otherwise it
   blocks with the current timeout value. */
int
sio_poll (struct siobuf *sio, int want_read, int want_write, int fast)
{
  int npoll, status, rval;
  struct pollfd pollfd[2];

  assert (sio != NULL);

  if (want_read && sio->read_unread > 0)
    return SIO_READ;
#ifdef USE_TLS
  /* SSL_read() returns data a record at a time, however it is possible
     that more than one record was read from the socket.  If this happens
     poll() will not report data waiting to be read but SSL_read() will
     return the next record.  Using SSL_pending() solves this problem.
   */
  if (want_read && sio->ssl != NULL && SSL_pending (sio->ssl))
    return SIO_READ;
#endif

  npoll = 0;
  if (want_read)
    {
      pollfd[npoll].fd = sio->sdr;
      pollfd[npoll].events = POLLIN;
      pollfd[npoll].revents = 0;
      npoll += 1;
    }
  if (want_write)
    {
      pollfd[npoll].fd = sio->sdw;
      pollfd[npoll].events = POLLOUT;
      pollfd[npoll].revents = 0;
      npoll += 1;
    }
  if (npoll == 0)
    return 0;

  while ((status = poll (pollfd, npoll, fast ? 0 : sio->milliseconds)) < 0)
    if (errno != EINTR)
      return -1;

  /* Timeout is not an error on the fast poll */
  if (status == 0 && fast)
    return 0;

  rval = 0;
  while (--npoll >= 0)
    {
      if (pollfd[npoll].revents & POLLIN)
	rval |= SIO_READ;
      if (pollfd[npoll].revents & POLLOUT)
	rval |= SIO_WRITE;
    }
  return (rval > 0) ? rval : -1;
}

#ifdef USE_TLS
static int
sio_sslpoll (struct siobuf *sio, int ret)
{
  int err, want_read, want_write;

  assert (sio != NULL);

  err = SSL_get_error (sio->ssl, ret);
  want_read = want_write = 0;
  if (err == SSL_ERROR_WANT_READ)
    want_read = 1;
  else if (err == SSL_ERROR_WANT_WRITE)
    want_write = 1;
  else
    return -1;
  return sio_poll (sio, want_read, want_write, 0);
}
#endif

void
sio_write (struct siobuf *sio, const void *bufp, int buflen)
{
  const char *buf = bufp;

  assert (sio != NULL && buf != NULL);

  if (buflen < 0)
    buflen = strlen (buf);
  if (buflen == 0)
    return;

  while (buflen > sio->write_available)
    {
      if (sio->write_available > 0)
	{
	  memcpy (sio->write_position, buf, sio->write_available);
	  sio->write_position += sio->write_available;
	  buf += sio->write_available;
	  buflen -= sio->write_available;
	}
      sio_flush (sio);
      assert (sio->write_available > 0);
    }
  if (buflen > 0)
    {
      memcpy (sio->write_position, buf, buflen);
      sio->write_position += buflen;
      sio->write_available -= buflen;
      /* If the buffer is exactly filled, flush it */
      if (sio->write_available == 0)
	sio_flush (sio);
    }
}

static void
raw_write (struct siobuf *sio, const char *buf, int len)
{
  int n, total, status;
  struct pollfd pollfd;

  assert (sio != NULL && buf != NULL);

  for (total = 0; total < len; total += n)
#ifdef USE_TLS
    if (sio->ssl != NULL)
      {
	/* SSL_write() writes a record a time.	The outer loop calls
	   it repeatedly until all the write buffer contents have
	   been written.  The inner loop handles EAGAIN (EWOULDBLOCK)
	   propagating up through OpenSSL. */
	while ((n = SSL_write (sio->ssl, buf, len)) <= 0)
	  if (sio_sslpoll (sio, n) <= 0)
	    return;
      }
    else
#endif
      {
        /* Its conceiveable that write() actually writes less than
           requested.  The outer loop calls this until all of the write
           buffer has been written.  The inner loop handles blocking
           in poll() and errors */
	pollfd.fd = sio->sdw;
	pollfd.events = POLLOUT;
	errno = 0;
	while ((n = write (sio->sdw, buf + total, len - total)) < 0)
	  {
	    if (errno == EINTR)
	      continue;
	    if (errno != EAGAIN)
	      return;

	    pollfd.revents = 0;
	    while ((status = poll (&pollfd, 1, sio->milliseconds)) < 0)
	      if (errno != EINTR)
		return;
	    if (status == 0)
	      {
	        errno = ETIMEDOUT;
		return;
	      }
	    if (!(pollfd.revents & POLLOUT))
	      return;
	    errno = 0;
	  }
      }
}

void
sio_flush (struct siobuf *sio)
{
  int length;

  assert (sio != NULL);

  if (sio->flush_mark != NULL && sio->flush_mark > sio->write_buffer)
    length = sio->flush_mark - sio->write_buffer;
  else
    length = sio->write_position - sio->write_buffer;
  if (length <= 0)
    return;

  if (sio->monitor_cb != NULL)
    (*sio->monitor_cb) (sio->write_buffer, length, 1, sio->cbarg);

  if (sio->encode_cb != NULL)
    {
      char *buf;
      int len;

      /* Rules for the encode callback.

         The output variables (here buf and len) may be set to the
         write_buffer iff the encoding can be performed in place and
         the result is shorter than the original data.  Otherwise the
         callback must maintain its own buffer which must persist until
         the next call in the same thread.  The secarg argument may be
         used to maintain this buffer. */
      (*sio->encode_cb) (&buf, &len, sio->write_buffer, length, sio->secarg);
      raw_write (sio, buf, len);
    }
  else
    raw_write (sio, sio->write_buffer, length);

  if (sio->flush_mark != NULL && sio->flush_mark > sio->write_buffer)
    {
      length = sio->write_position - sio->flush_mark;
      if (length > 0)
        memmove (sio->write_buffer, sio->flush_mark, length);
    }
  else
    length = 0;
  sio->write_available = sio->buffer_size - length;
  sio->write_position = sio->write_buffer + length;
  sio->flush_mark = NULL;
}

void
sio_mark (struct siobuf *sio)
{
  assert (sio != NULL);

  sio->flush_mark = sio->write_position;
}

/* N.B. raw_read() requires a non-blocking read, otherwise it would
   block indefinitely instead of timing out.  Normally the poll()
   should not be needed since the protocol level will have polled
   before reading or writing. */
static int
raw_read (struct siobuf *sio, char *buf, int len)
{
  int n, status;
  struct pollfd pollfd;

  assert (sio != NULL && buf != NULL && len > 0);

#ifdef USE_TLS
  if (sio->ssl != NULL)
    {
      /* SSL_read() reads complete records from the network and returns
	 one record at a time.  This means that poll() may indicate that
	 there is no data waiting to be read even though SSL_read() will
	 return the next record.  SSL_pending() is used to avoid this
	 problem. The loop handles EAGAIN (EWOULDBLOCK) propagating up
	 through OpenSSL. */
      while ((n = SSL_read (sio->ssl, buf, len)) < 0)
        if (sio_sslpoll (sio, n) <= 0)
	  break;
    }
  else
#endif
    {
      pollfd.fd = sio->sdr;
      pollfd.events = POLLIN;
      errno = 0;
      while ((n = read (sio->sdr, buf, len)) < 0)
	{
	  if (errno == EINTR)
	    continue;
	  if (errno != EAGAIN)
	    return 0;

	  pollfd.revents = 0;
	  while ((status = poll (&pollfd, 1, sio->milliseconds)) < 0)
	    if (errno != EINTR)
	      return 0;
	  if (status == 0)
	    {
	      errno = ETIMEDOUT;
	      return 0;
	    }
	  if (!(pollfd.revents & POLLIN))
	    return 0;
	  errno = 0;
	}
    }
  return n;
}

int
sio_fill (struct siobuf *sio)
{
  assert (sio != NULL);

  sio->read_unread = raw_read (sio, sio->read_buffer, sio->buffer_size);
  if (sio->read_unread <= 0)
    return 0;

  if (sio->decode_cb != NULL)
    /* Rules for the decode callback.

       The output variables (here buf and len) may be set to the
       read_buffer iff the decoding can be performed in place and
       the result is shorter than the original data.  Otherwise the
       callback must maintain its own buffer which must persist until
       the next call in the same thread.  The secarg argument may be
       used to maintain this buffer. */
    (*sio->decode_cb) (&sio->read_position, &sio->read_unread,
		       sio->read_buffer, sio->read_unread, sio->secarg);
  else
    sio->read_position = sio->read_buffer;

  if (sio->monitor_cb != NULL && sio->read_unread > 0)
    (*sio->monitor_cb) (sio->read_position, sio->read_unread,
			0, sio->cbarg);
  return sio->read_unread > 0;
}

int
sio_read (struct siobuf *sio, void *bufp, int buflen)
{
  char *buf = bufp;
  int count, total;

  assert (sio != NULL && buf != NULL && buflen > 0);

  if (sio->read_unread <= 0 && !sio_fill (sio))
    return -1;

  total = 0;
  do
    while (sio->read_unread > 0)
      {
        if ((count = sio->read_unread) > buflen)
          count = buflen;
        memcpy (buf, sio->read_position, count);
        sio->read_position += count;
        sio->read_unread -= count;

        total += count;
        if ((buflen -= count) <= 0)
          return total;
        buf += count;
      }
  while (sio_fill (sio));
  return total;
}

char *
sio_gets (struct siobuf *sio, char buf[], int buflen)
{
  int c;
  char *p;

  assert (sio != NULL && buf != NULL && buflen > 0);

  if (sio->read_unread <= 0 && !sio_fill (sio))
    return NULL;

  p = buf;
  do
    while (sio->read_unread > 0)
      {
	c = *sio->read_position++;
	sio->read_unread--;
	*p++ = c;
	buflen--;
	if (c == '\n' || buflen <= 1)
	  {
	    *p = '\0';
	    return buf;
	  }
      }
  while (sio_fill (sio));
  *p = '\0';
  return buf;
}

void *
sio_set_userdata (struct siobuf *sio, void *user_data)
{
  void *old = sio->user_data;

  sio->user_data = user_data;
  return old;
}

void *
sio_get_userdata (struct siobuf *sio)
{
  return sio->user_data;
}

int
sio_printf (struct siobuf *sio, const char *format, ...)
{
  va_list alist;
  char buf[1024];
  int len;

  assert (sio != NULL && format != NULL);

  va_start (alist, format);
  len = vsnprintf (buf, sizeof buf, format, alist);
  va_end (alist);
  if (len >= (int) sizeof buf - 1)
    len = sizeof buf - 1;
  if (len > 0)
    sio_write (sio, buf, len);
  return len;
}
