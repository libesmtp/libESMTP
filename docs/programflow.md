# API Overview

A libESMTP application should be structured similarly to the following.
Declarations, function arguments and other details are omitted for clarity.

A working, fully commented, example application may be found in
`examples/mail-file.c` in the distribution.

``` c
void
libesmtp_application ()
{
  /* Create the libESMTP session */
  session = smtp_create_session ();

  /* Add a message. */
  message = smtp_add_message (session);

  /* Add recipients to the message. */
  while ((address = get_recipient ()) != NULL)
    {
      recipient = smtp_add_recipient (message, address);

      /* set a recipient option, e.g. to request a DSN if anything fails */
      smtp_dsn_set_notify (recipient, Notify_DELAY | Notify_FAILURE);
    }

  /* A callback to read the message from the application is required */
  smtp_set_messagecb (message, message_cb, arg);

  /* Add a header in case the application does not supply one via the
     message callback */
  smtp_set_header (message, "To", "Joe Bloggs", "joe.bloggs@example.org");

  /* Use TLS to secure the session */
  smtp_starttls_enable (session, Starttls_REQUIRED);

  /* Set the submission host (aka smarthost) */
  smtp_set_server (session, "msa.example.org:587");

  /* set an event callback and start the session */
  smtp_set_eventcb (session, event_cb, NULL);
  smtp_start_session (session);

  /* tear down the session */
  smtp_destroy_session (session);
}
```

Some points to note are that multiple messages may be added to a session and
each message may have multiple recipients.

The event callback `event_cb` is called as the SMTP protocol progresses and the
message callback `message_cb` is used to read the message from the application.

`smtp_start_session()` returns an error code if anything fails, however take
note that it returns successfully if the SMTP protocol progresses correctly,
even if the server refuses messages or recipients, for example if a message is
too large or it cannot deliver to a particular recipient, libESMTP still
considers that the protocol operated correctly.  It is necessary to use the
event callback to determine if messages or recipients are refused by the
server. Alternatively it is possible to enumerate the messages and recipients
to examine the status codes if `smtp_start_session()` returns successfully.

The message callback should supply a properly formatted RFC 5322 message, that
is headers followed by a blank line followed by the message body.  Each line
*must* be terminated with a CR-LF, CR or LF on its own will not be recognised.
If the application supplies no headers and relies on libESMTP it must still
precede the message with a blank line.

The message recipients listed in the 'To:', 'Cc:' and 'Bcc:' message headers
need not correspond to those specified in the SMTP protocol using the RCPT TO
verb. Similarly the 'From:' message header need not correspond to that
specified in the MAIL FROM verb. This might occur if the message has been
distributed through mailing list software.
