/*
 *  A libESMTP Example Application.
 *  Copyright (C) 2001,2002,2021  Brian Stafford <https://libesmtp.github.io/>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/* This program accepts a single file argument followed by a list of
   recipients.  The file is mailed to each of the recipients.

   Error checking is minimal to non-existent, this is just a quick
   and dirty program to give a feel for using libESMTP.
 */
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#include <openssl/ssl.h>
#include <auth-client.h>
#include <libesmtp.h>

#if !defined (__GNUC__) || __GNUC__ < 2
# define __attribute__(x)
#endif
#define unused      __attribute__((unused))

enum { TO = 10, CC, BCC, };

struct option longopts[] =
  {
    { "help", no_argument, NULL, '?', }, 
    { "version", no_argument, NULL, 'v', }, 
    { "host", required_argument, NULL, 'h', }, 
    { "monitor", no_argument, NULL, 'm', }, 
    { "crlf", no_argument, NULL, 'c', }, 
    { "notify", required_argument, NULL, 'n', }, 
    { "mdn", no_argument, NULL, 'd', }, 
    { "subject", required_argument, NULL, 's', }, 
    { "reverse-path", required_argument, NULL, 'f', }, 
    { "tls", no_argument, NULL, 't', }, 
    { "require-tls", no_argument, NULL, 'T', }, 
    { "noauth", no_argument, NULL, 1, }, 

    { "to", required_argument, NULL, TO, },
    { "cc", required_argument, NULL, CC, },
    { "bcc", required_argument, NULL, BCC, },

    { NULL, 0, NULL, 0, },
  };

const char *readlinefp_cb (void **buf, int *len, void *arg);
void monitor_cb (const char *buf, int buflen, int writing, void *arg);
void print_recipient_status (smtp_recipient_t recipient,
			     const char *mailbox, void *arg);
int authinteract (auth_client_request_t request, char **result, int fields,
                  void *arg);
int tlsinteract (char *buf, int buflen, int rwflag, void *arg);
int handle_invalid_peer_certificate(long vfy_result);
void event_cb (smtp_session_t session, int event_no, void *arg, ...);
void usage (void);
void version (void);
/* FIXME getpass() is obsolete - previously declared in unistd.h */
extern char *getpass(const char *prompt);

int
main (int argc, char **argv)
{
  smtp_session_t session;
  smtp_message_t message;
  smtp_recipient_t recipient;
  auth_context_t authctx;
  const smtp_status_t *status;
  struct sigaction sa;
  char *host = NULL;
  char *from = NULL;
  char *subject = NULL;
  int nocrlf = 0;
  int noauth = 0;
  int to_cc_bcc = 0;
  char *file;
  FILE *fp;
  int c;
  enum notify_flags notify = Notify_NOTSET;

  /* This program sends only one message at a time.  Create an SMTP
     session and add a message to it. */
  auth_client_init ();
  session = smtp_create_session ();
  message = smtp_add_message (session);

  while ((c = getopt_long (argc, argv, "dmch:f:s:n:tTv",
  			   longopts, NULL)) != EOF)
    switch (c)
      {
      case 'h':
        host = optarg;
        break;

      case 'f':
        from = optarg;
        break;

      case 's':
        subject = optarg;
        break;

      case 'c':
        nocrlf = 1;
        break;

      case 'm':
        smtp_set_monitorcb (session, monitor_cb, stdout, 1);
        break;

      case 'n':
        if (strcmp (optarg, "success") == 0)
          notify |= Notify_SUCCESS;
        else if (strcmp (optarg, "failure") == 0)
          notify |= Notify_FAILURE;
        else if (strcmp (optarg, "delay") == 0)
          notify |= Notify_DELAY;
        else if (strcmp (optarg, "never") == 0)
          notify = Notify_NEVER;
        break;

      case 'd':
        /* Request MDN sent to the same address as the reverse path */
        smtp_set_header (message, "Disposition-Notification-To", NULL, NULL);
        break;

      case 't':
        smtp_starttls_enable (session, Starttls_ENABLED);
	break;

      case 'T':
        smtp_starttls_enable (session, Starttls_REQUIRED);
	break;
    
      case 'v':
        version ();
        exit (2);

      case 1:
        noauth = 1;
        break;

      case TO:
        smtp_set_header (message, "To", NULL, optarg);
        to_cc_bcc = 1;
        break;
      case CC:
        smtp_set_header (message, "Cc", NULL, optarg);
        to_cc_bcc = 1;
        break;
      case BCC:
        smtp_set_header (message, "Bcc", NULL, optarg);
        to_cc_bcc = 1;
        break;

      default:
        usage ();
        exit (2);
      }

  /* At least two more arguments are needed.
   */
  if (optind > argc - 2)
    {
      usage ();
      exit (2);
    }

  /* NB.  libESMTP sets timeouts as it progresses through the protocol.
     In addition the remote server might close its socket on a timeout.
     Consequently libESMTP may sometimes try to write to a socket with
     no reader.  Ignore SIGPIPE, then the program doesn't get killed
     if/when this happens. */
  sa.sa_handler = SIG_IGN;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction (SIGPIPE, &sa, NULL); 

  /* Set the host running the SMTP server.  LibESMTP has a default port
     number of 587, however this is not widely deployed so the port
     is specified as 25 along with the default MTA host. */
  smtp_set_server (session, host ? host : "localhost:25");

  /* Do what's needed at application level to use authentication.
   */
  authctx = auth_create_context ();
  auth_set_mechanism_flags (authctx, AUTH_PLUGIN_PLAIN, 0);
  auth_set_interact_cb (authctx, authinteract, NULL);

  /* Use our callback for X.509 certificate passwords.  If STARTTLS is
     not in use or disabled in configure, the following is harmless. */
  smtp_starttls_set_password_cb (tlsinteract, NULL);
  smtp_set_eventcb(session, event_cb, NULL);

  /* Now tell libESMTP it can use the SMTP AUTH extension.
   */
  if (!noauth)
    smtp_auth_set_context (session, authctx);

  /* Set the reverse path for the mail envelope.  (NULL is ok)
   */
  smtp_set_reverse_path (message, from);

#if 0
  /* The message-id is OPTIONAL but SHOULD be present.  By default
     libESMTP supplies one.  If this is not desirable, the following
     prevents one making its way to the server.
     N.B. It is not possible to prohibit REQUIRED headers.  Furthermore,
          the submission server will probably add a Message-ID header,
          so this cannot prevent the delivered message from containing
          the message-id.  */
  smtp_set_header_option (message, "Message-Id", Hdr_PROHIBIT, 1);
#endif

  /* RFC 2822 doesn't require recipient headers but a To: header would
     be nice to have if not present. */
  if (!to_cc_bcc)
    smtp_set_header (message, "To", NULL, NULL);

  /* Set the Subject: header.  For no reason, we want the supplied subject
     to override any subject line in the message headers. */
  if (subject != NULL)
    {
      smtp_set_header (message, "Subject", subject);
      smtp_set_header_option (message, "Subject", Hdr_OVERRIDE, 1);
    }
  
  /* Open the message file and set the callback to read it.
   */
  file = argv[optind++];
  if (strcmp (file, "-") == 0)
    fp = stdin;
  else if ((fp = fopen (file, "r")) == NULL)
    {
      fprintf (stderr, "can't open %s: %s\n", file, strerror (errno));
      exit (1);
    }
  if (nocrlf)
    smtp_set_messagecb (message, readlinefp_cb, fp);
  else
    smtp_set_message_fp (message, fp);

  /* Add remaining program arguments as message recipients.
   */
  while (optind < argc)
    {
      recipient = smtp_add_recipient (message, argv[optind++]);

      /* Recipient options set here */
      if (notify != Notify_NOTSET)
        smtp_dsn_set_notify (recipient, notify);
    }

  /* Initiate a connection to the SMTP server and transfer the
     message. */
  if (!smtp_start_session (session))
    {
      char buf[128];

      fprintf (stderr, "SMTP server problem %s\n",
	       smtp_strerror (smtp_errno (), buf, sizeof buf));
    }
  else
    {
      /* Report on the success or otherwise of the mail transfer.
       */
      status = smtp_message_transfer_status (message);
      printf ("%d %s", status->code,
              (status->text != NULL) ? status->text : "\n");
      smtp_enumerate_recipients (message, print_recipient_status, NULL);
    }

  /* Free resources consumed by the program.
   */
  smtp_destroy_session (session);
  auth_destroy_context (authctx);
  fclose (fp);
  auth_client_exit ();
  exit (0);
}

/* Callback to prnt the recipient status */
void
print_recipient_status (smtp_recipient_t recipient,
			const char *mailbox, void *arg unused)
{
  const smtp_status_t *status;

  status = smtp_recipient_status (recipient);
  printf ("%s: %d %s", mailbox, status->code, status->text);
}

/* Callback function to read the message from a file.  Since libESMTP
   does not provide callbacks which translate line endings, one must
   be provided by the application.

   The message is read a line at a time and the newlines converted
   to \r\n.  Unfortunately, RFC 822 states that bare \n and \r are
   acceptable in messages and that individually they do not constitute a
   line termination.  This requirement cannot be reconciled with storing
   messages with Unix line terminations.  RFC 2822 rescues this situation
   slightly by prohibiting lone \r and \n in messages.

   The following code cannot therefore work correctly in all situations.
   Furthermore it is very inefficient since it must search for the \n.
 */
#define BUFLEN	8192

const char *
readlinefp_cb (void **buf, int *len, void *arg)
{
  int octets;

  if (*buf == NULL)
    *buf = malloc (BUFLEN);

  if (len == NULL)
    {
      rewind ((FILE *) arg);
      return NULL;
    }

  if (fgets (*buf, BUFLEN - 2, (FILE *) arg) == NULL)
    octets = 0;
  else
    {
      char *p = strchr (*buf, '\0');

      if (p[-1] == '\n' && p[-2] != '\r')
        {
	  strcpy (p - 1, "\r\n");
	  p++;
        }
      octets = p - (char *) *buf;
    }
  *len = octets;
  return *buf;
}

void
monitor_cb (const char *buf, int buflen, int writing, void *arg)
{
  FILE *fp = arg;

  if (writing == SMTP_CB_HEADERS)
    {
      fputs ("H: ", fp);
      fwrite (buf, 1, buflen, fp);
      return;
    }

 fputs (writing ? "C: " : "S: ", fp);
 fwrite (buf, 1, buflen, fp);
 if (buf[buflen - 1] != '\n')
   putc ('\n', fp);
}


/* Callback to request user/password info.  Not thread safe. */
int
authinteract (auth_client_request_t request, char **result, int fields,
              void *arg unused)
{
  char prompt[64];
  static char resp[512];
  char *p, *rp;
  int i, n, tty;

  rp = resp;
  for (i = 0; i < fields; i++)
    {
      n = snprintf (prompt, sizeof prompt, "%s%s: ", request[i].prompt,
		    (request[i].flags & AUTH_CLEARTEXT) ? " (not encrypted)"
		    					: "");
      if (request[i].flags & AUTH_PASS)
	result[i] = getpass (prompt);
      else
	{
	  tty = open ("/dev/tty", O_RDWR);
	  if (write (tty, prompt, n) != n)
	    {
	    }
	  n = read (tty, rp, sizeof resp - (rp - resp));
	  close (tty);
	  p = rp + n;
	  while (isspace (p[-1]))
	    p--;
	  *p++ = '\0';
	  result[i] = rp;
	  rp = p;
	}
    }
  return 1;
}

int
tlsinteract (char *buf, int buflen, int rwflag unused, void *arg unused)
{
  char *pw;
  int len;

  pw = getpass ("certificate password");
  len = strlen (pw);
  if (len + 1 > buflen)
    return 0;
  strcpy (buf, pw);
  return len;
}

int
handle_invalid_peer_certificate(long vfy_result)
{
  const char *k ="rare error";
  switch(vfy_result) {
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    k="X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT"; break;
  case X509_V_ERR_UNABLE_TO_GET_CRL:
    k="X509_V_ERR_UNABLE_TO_GET_CRL"; break;
  case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
    k="X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE"; break;
  case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
    k="X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE"; break;
  case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
    k="X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY"; break;
  case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    k="X509_V_ERR_CERT_SIGNATURE_FAILURE"; break;
  case X509_V_ERR_CRL_SIGNATURE_FAILURE:
    k="X509_V_ERR_CRL_SIGNATURE_FAILURE"; break;
  case X509_V_ERR_CERT_NOT_YET_VALID:
    k="X509_V_ERR_CERT_NOT_YET_VALID"; break;
  case X509_V_ERR_CERT_HAS_EXPIRED:
    k="X509_V_ERR_CERT_HAS_EXPIRED"; break;
  case X509_V_ERR_CRL_NOT_YET_VALID:
    k="X509_V_ERR_CRL_NOT_YET_VALID"; break;
  case X509_V_ERR_CRL_HAS_EXPIRED:
    k="X509_V_ERR_CRL_HAS_EXPIRED"; break;
  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    k="X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD"; break;
  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    k="X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD"; break;
  case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
    k="X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD"; break;
  case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
    k="X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD"; break;
  case X509_V_ERR_OUT_OF_MEM:
    k="X509_V_ERR_OUT_OF_MEM"; break;
  case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    k="X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT"; break;
  case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    k="X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN"; break;
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    k="X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY"; break;
  case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    k="X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE"; break;
  case X509_V_ERR_CERT_CHAIN_TOO_LONG:
    k="X509_V_ERR_CERT_CHAIN_TOO_LONG"; break;
  case X509_V_ERR_CERT_REVOKED:
    k="X509_V_ERR_CERT_REVOKED"; break;
  case X509_V_ERR_INVALID_CA:
    k="X509_V_ERR_INVALID_CA"; break;
  case X509_V_ERR_PATH_LENGTH_EXCEEDED:
    k="X509_V_ERR_PATH_LENGTH_EXCEEDED"; break;
  case X509_V_ERR_INVALID_PURPOSE:
    k="X509_V_ERR_INVALID_PURPOSE"; break;
  case X509_V_ERR_CERT_UNTRUSTED:
    k="X509_V_ERR_CERT_UNTRUSTED"; break;
  case X509_V_ERR_CERT_REJECTED:
    k="X509_V_ERR_CERT_REJECTED"; break;
  }
  printf("SMTP_EV_INVALID_PEER_CERTIFICATE: %ld: %s\n", vfy_result, k);
  return 1; /* Accept the problem */
}

void event_cb (smtp_session_t session unused, int event_no, void *arg,...)
{
  va_list alist;
  int *ok;

  va_start(alist, arg);
  switch(event_no) {
  case SMTP_EV_CONNECT: 
  case SMTP_EV_MAILSTATUS:
  case SMTP_EV_RCPTSTATUS:
  case SMTP_EV_MESSAGEDATA:
  case SMTP_EV_MESSAGESENT:
  case SMTP_EV_DISCONNECT: break;
  case SMTP_EV_WEAK_CIPHER: {
    int bits;
    bits = va_arg(alist, long); ok = va_arg(alist, int*);
    printf("SMTP_EV_WEAK_CIPHER, bits=%d - accepted.\n", bits);
    *ok = 1; break;
  }
  case SMTP_EV_STARTTLS_OK:
    puts("SMTP_EV_STARTTLS_OK - TLS started here."); break;
  case SMTP_EV_INVALID_PEER_CERTIFICATE: {
    long vfy_result;
    vfy_result = va_arg(alist, long); ok = va_arg(alist, int*);
    *ok = handle_invalid_peer_certificate(vfy_result);
    break;
  }
  case SMTP_EV_NO_PEER_CERTIFICATE: {
    ok = va_arg(alist, int*); 
    puts("SMTP_EV_NO_PEER_CERTIFICATE - accepted.");
    *ok = 1; break;
  }
  case SMTP_EV_WRONG_PEER_CERTIFICATE: {
    ok = va_arg(alist, int*);
    puts("SMTP_EV_WRONG_PEER_CERTIFICATE - accepted.");
    *ok = 1; break;
  }
  case SMTP_EV_NO_CLIENT_CERTIFICATE: {
    ok = va_arg(alist, int*); 
    puts("SMTP_EV_NO_CLIENT_CERTIFICATE - accepted.");
    *ok = 1; break;
  }
  default:
    printf("Got event: %d - ignored.\n", event_no);
  }
  va_end(alist);
}

void
usage (void)
{
  fputs ("Copyright © 2001,2002,2021 Brian Stafford <https://libesmtp.github.io/>\n"
	 "\n"
	 "This program is free software; you can redistribute it and/or modify\n"
	 "it under the terms of the GNU General Public License as published\n"
	 "by the Free Software Foundation; either version 2 of the License,\n"
	 "or (at your option) any later version.\n"
	 "\n"
	 "This program is distributed in the hope that it will be useful,\n"
	 "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	 "GNU General Public License for more details.\n"
	 "\n"
	 "You should have received a copy of the GNU General Public License\n"
         "along with this program. If not, see <https://www.gnu.org/licenses/>.\n"
	 "\n"
         "usage: mail-file [options] file mailbox [mailbox ...]\n"
         "\t-h --host hostname[:service] -- set SMTP host and service (port)\n"
         "\t-f --reverse-path mailbox -- set reverse path\n"
         "\t-s --subject text -- set subject of the message\n"
         "\t-n --notify success|failure|delay|never -- request DSN\n"
         "\t-d --mdn -- request MDN\n"
         "\t-m --monitor -- watch the protocol session with the server\n"
         "\t-c --crlf -- translate line endings from \\n to CR-LF\n"
         "\t-t --tls -- use STARTTLS extension if possible\n"
         "\t-T --require-tls -- require use of STARTTLS extension\n"
         "\t   --noauth -- do not attempt to authenticate to the MSA\n"
         "\t   --version -- show version info and exit\n"
         "\t   --help -- this message\n"
         "\n"
         "Specify the file argument as \"-\" to read standard input.\n"
         "The input must be in RFC 5322 format, that is it must consist\n"
         "of a sequence of message headers terminated by a blank line and\n"
         "followed by the message body.  Lines must be terminated with the\n"
         "canonic CR-LF sequence unless the --crlf flag is specified.\n"
         "Total line length must not exceed 1000 characters.\n",
         stderr);
}

void
version (void)
{
  char buf[32];

  smtp_version (buf, sizeof buf, 0);
  printf ("libESMTP version %s\n", buf);
}
