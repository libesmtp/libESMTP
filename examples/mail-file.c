/*
 *  A libESMTP Example Application.
 *  Copyright (C) 2001  Brian Stafford <brian@stafford.uklinux.net>
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This program accepts a single file argument followed by a list of
   recipients.  The file is mailed to each of the recipients.

   Error checking is minimal to non-existent, this is just a quick
   and dirty program to give a feel for using libESMTP.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <auth-client.h>
#include <libesmtp.h>

struct option longopts[] =
  {
    { "help", no_argument, NULL, '?', }, 
    { "host", required_argument, NULL, 'h', }, 
    { "monitor", no_argument, NULL, 'm', }, 
    { "no-crlf", no_argument, NULL, 'c', }, 
    { "notify", required_argument, NULL, 'n', }, 
    { "mdn", no_argument, NULL, 'd', }, 
    { "subject", required_argument, NULL, 's', }, 
    { "reverse-path", required_argument, NULL, 'f', }, 
    { NULL, },
  };

const char *readfp_cb (void **buf, int *len, void *arg);
const char *readlinefp_cb (void **buf, int *len, void *arg);
void monitor_cb (const char *buf, int buflen, int writing, void *arg);
void print_recipient_status (smtp_recipient_t recipient,
			     const char *mailbox, void *arg);
int authinteract (auth_client_request_t request, char **result, int fields,
                  void *arg);
void usage (void);

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
  char *file;
  FILE *fp;
  int c;
  enum notify_flags notify = Notify_NOTSET;

  /* This program sends only one message at a time.  Create an SMTP
     session and add a message to it. */
  auth_client_init ();
  session = smtp_create_session ();
  message = smtp_add_message (session);

  while ((c = getopt_long (argc, argv, "dmch:f:s:n:",
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
  sa.sa_flags = SA_NOMASK;
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

  /* Now tell libESMTP it can use the SMTP AUTH extension.
   */
  smtp_auth_set_context (session, authctx);

  /* Set the reverse path for the mail envelope.  (NULL is ok)
   */
  smtp_set_reverse_path (message, from);

  /* RFC 822 doesn't actually require a message to have a Message-Id:
     header.  libESMTP can generate one.  This is how to request the
     library to do this */
  smtp_set_header (message, "Message-Id", NULL);

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
  smtp_set_messagecb (message, nocrlf ? readlinefp_cb : readfp_cb, fp);

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
  smtp_start_session (session);

  /* Report on the success or otherwise of the mail transfer.
   */
  status = smtp_message_transfer_status (message);
  printf ("%d %s", status->code, status->text);
  smtp_enumerate_recipients (message, print_recipient_status, NULL);

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
			const char *mailbox, void *arg)
{
  const smtp_status_t *status;

  status = smtp_recipient_status (recipient);
  printf ("%s: %d %s", mailbox, status->code, status->text);
}

/* Callback function to read the message from a file.  The file MUST be
   formatted according to RFC 822 and lines MUST be terminated with the
   canonical CRLF sequence.

   To make this example program into a useful one, a more sophisticated
   callback capable of translating the Un*x \n tro CRLF would be needed.
 */
#define BUFLEN	8192

const char *
readfp_cb (void **buf, int *len, void *arg)
{
  if (*buf == NULL)
    *buf = malloc (BUFLEN);

  if (len == NULL)
    {
      rewind ((FILE *) arg);
      return NULL;
    }

  *len = fread (*buf, 1, BUFLEN, (FILE *) arg);
  return *buf;
}

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

  /* The message needs to be read a line at a time and the newlines
     converted to \r\n.  Unfortunately, RFC 822 states that bare \n and
     \r are acceptable in messages and that individually they do not
     constiture a line termination.  This requirement cannot be
     reconciled with storing messages with Unix line terminations.

     The following code cannot therefore work correctly in all situations.
     Furthermore it is very inefficient since it must search for the \n.
   */
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
              void *arg)
{
  char prompt[64];
  static char resp[64];
  char *p;
  int i;

  for (i = 0; i < fields; i++)
    {
      sprintf (prompt, "%s%s: ", request[i].prompt,
               (request[i].flags & AUTH_CLEARTEXT) ? " (not encrypted)" : "");
      if (request[i].flags & AUTH_PASS)
	result[i] = getpass (prompt);
      else
	{
	  fputs (prompt, stderr);
	  fgets (resp, sizeof resp, stdin);
	  p = strchr (resp, '\0');
	  while (isspace (p[-1]))
	    p--;
	  *p = '\0';
	  result[i] = resp;
	}
    }
  return 1;
}

void
usage (void)
{
  fputs ("Copyright (C) 2001  Brian Stafford <brian@stafford.uklinux.net>\n"
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
	 "along with this program; if not, write to the Free Software\n"
	 "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
	 "\n"
         "usage: mail-file [options] file mailbox [mailbox ...]\n"
         "\t-h,--host=hostname[:port] -- set SMTP server\n"
         "\t-f,--reverse-path=mailbox -- set reverse path\n"
         "\t-s,--subject=text -- set subject of the message\n"
         "\t-n,--notify=success|failure|delay|never -- request DSN\n"
         "\t-d,--mdn -- request MDN\n"
         "\t-m,--monitor -- watch the protocol session with the server\n"
         "\t--help -- this message\n",
         stderr);
}

