# TLS Certificates

## Initialisation

When libESMTP initialises an OpenSSL SSL_CTX it loads certificates and private
keys from files below the `$HOME/.authenticate` directory as follows.

File or sub-directory | Description
----------------------|------------
private/smtp-starttls.pem | user's certificate and private key
ca/ | user's hashed CA directory
ca.pem | user's trusted CA list

If neither the `ca` directory nor `ca.pem` exist, the OpenSSL default locations
are used instead,

If the user's certificate is protected by a password, it is requested when the
SSL_CTX is initialised. It is not necessary to provide a client certificate,
however if one is supplied it is presented to the server only if requested.
Note that certain servers may refuse to accept mail unless a client certificate
is available.

The SSL_CTX created by libESMTP supports only TLSv1 protocols and higher.

## Permissions

`.authenticate` and its subdirectories must be owned and readable only by their
real owner; group and other permissions must not be set. That is, directories
should have the mode __0700__ or __0500__ and files should be regular with mode
__0400__ or __0600__.  File status is checked using the `stat()` system call so
symbolic or hard links may be used to avoid duplication where necessary.

If permissions and file ownership do not follow these rules, libESMTP ignores
the affected files.  If certificate files are protected by passwords, libESMTP
will request the password using a callback function when needed.

## Alternative SSL_CTX Initialisation

An application may wish to supply its own SSL_CTX instead of using the one
automatically created by libESMTP, for example to use SSL or TLS protocols not
supported by libESMTP. This may done using [`smtp_starttls_set_ctx()`][1].
This must be done before calling [`smtp_start_session()`][2].

The certificate file layout described above is not used in this case.

## Connection

When libESMTP is about to switch to TLS when processing the STARTTLS verb, it
checks if a file `$HOME/.authenticate/$HOST/private/smtp-starttls.pem` exists,
where `$HOST` is the hostname of the SMTP server.  If it exists, it is used
instead of the default user certificate; the password is requested at this
time.  if the certificate is protected by a password. This is done even when
using an application supplied SSL_CTX.

Note that some servers may require further authentication, even if a
certificate is presented.

## Server Certificate Validation

libESMTP checks the server certificate for acceptable security. First the
certificate is checked for validity against the CA list.

Next, libESMTP checks that the cipher is sufficeintly strong.  In this case
*sufficiently strong* means that the cipher has at least a 40 bit key.  Recent
versions of the TLS protocol never negotiate such a weak key, however an
application supplied SSL_CTX may support obsolete SSL versions.

If the server certificate has a subjectAltName field, libESMTP checks that the
server's domain name matches one of the subjectAltNames.  If a subjectAltName
is not present, the certificate's commonName is used.

Domain names are compared using the wildcard match described in RFC 2818.

[1]: smtp-tls.html#c.smtp_starttls_set_ctx
[2]: smtp-api.html#c.smtp_start_session
