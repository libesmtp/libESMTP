# TLS Certificates

## Server Certificate Validation

When libESMTP connects to an upstream MTA and is configured to use TLS
it checks the server certificate for acceptable security. First the
certificate is checked for validity against the CA list.

Next, libESMTP checks that the cipher is sufficeintly strong.  In this case
*sufficiently strong* means that the cipher has at least a 40 bit key.  Recent
versions of the TLS protocol never negotiate such a weak key, however an
application supplied SSL_CTX may support obsolete SSL versions.

If the server certificate has a subjectAltName field, libESMTP checks that the
server's domain name matches one of the subjectAltNames.  If a subjectAltName
is not present, the certificate's commonName is used.

It is expected that the MTA's server certificate will be configured with the
host's canonic domain name.  Therefore when libESMTP connects to the MTA, it
retrieves the server's canonic domain name which is used to check the
certificate. The host's canonic name will differ only if the host name refers
to a CNAME record in the DNS.

Domain names are compared using the wildcard match described in RFC 2818.

## Client Certificate

During connection, libESMTP also checks if a client certificate is available
which is presented to the MTA on request. The client certificate can be
specific to a host or a fallback certificate presented.  It is not normally
necessary to provide a client certificate.

If client certificates are protected with a password, OpenSSL requests
passwords on the standard input unless a password callback is provided.
Therefore it is recommended that all applications set a callback using
`smtp_starttls_set_password_cb()`.  Alternatively an application SSL_CTX can be
configured for libESMTP to use via `smtp_starttls_set_ctx()`.


## Initialisation

Unless an application provides an OpenSSL SSL_CTX using
`smtp_starttls_set_ctx()` libESMTP initialises one when necessary.

From version 1.1.0, libESMTP follows [Open Desktop XDG][3] file layout
conventions; certificates and private keys are loaded as follows:

File or sub-directory | Description
----------------------|------------
$CONFIG/ca/ | user's hashed CA directory
$CONFIG/ca.pem | user's trusted CA list
$CONFIG/private/smtp-starttls.pem | user's certificate and private key
$CONFIG/private/$HOST.pem | host-specific certificate and private key

where `$CONFIG` is defined as if
``` sh
CONFIG=${XDG_CONFIG_HOME:-$HOME/.config}/libesmtp
```
were specified to the shell and `$HOST` is the canonic host name of the MTA.

Use the [`openssl rehash`][4] command to create the hashed CA directory
symbolic links if required.

### Legacy Convention

The deprecated libESMTP legacy file layout convention is as follows:

File or sub-directory | Description
----------------------|------------
$CONFIG/ca/ | user's hashed CA directory
$CONFIG/ca.pem | user's trusted CA list
$CONFIG/private/smtp-starttls.pem | user's certificate and private key
$CONFIG/$HOST/private/smtp-starttls.pem | host-specific certificate and private key

where `$CONFIG` is defined as if
``` sh
CONFIG=$HOME/.authenticate
```
were specified to the shell and `$HOST` is the canonic host name of the MTA.

libESMTP may be configured to follow the legacy convention at compile time.
Note that other than the location of `$CONFIG` the legacy and XDG conventions
differ in the filenames for host-specific client certificates. APIs and their
semantics are identical for both legacy and XDG layouts.

## OpenSSL Default Paths

If neither the `ca` directory nor `ca.pem` exist, the OpenSSL default locations
are used instead. This is recommended practice since the host system will
usually have an up-to-date list of Certificate Authorities installed as part of
its OpenSSL configuration.

## Certificate Passwords

If the user's certificate is protected by a password, it is requested when the
SSL_CTX is initialised. It is not necessary to provide a client certificate,
however if one is supplied it is presented to the server only if requested.
Note that certain servers may refuse to accept mail unless a client certificate
is available.

The SSL_CTX created by libESMTP supports only TLSv1 protocols and higher.

## Permissions

Certificate files and the `ca` directory must be owned and readable only by
their real owner; group and other permissions must not be set, that is, `ca`
must have mode __0700__ or __0500__ and certificate files should be regular
with mode __0400__ or __0600__.  File status is checked using the `stat()`
system call so symbolic or hard links may be used to avoid duplication where
necessary.  It is strongly recommended that other directories, in particular
`private` should have the mode __0700__ or __0500__.

If permissions and file ownership do not follow these rules, libESMTP ignores
the affected files.  If certificate files are protected by passwords, libESMTP
will request the password using a callback function when needed.

## Connection

When libESMTP is about to switch to TLS when processing the STARTTLS verb, it
checks if a host-specific client certificate exists with a pathname as defined
above.  If it exists, it is used instead of the default user certificate.

Note that some servers may require further authentication, even if a
certificate is presented. libESMTP supports the EXTERNAL SASL mechanism
using the server certificate's commonName.

## Alternative SSL_CTX Initialisation

An application may wish to supply its own SSL_CTX instead of using the one
automatically created by libESMTP, for example to use SSL or TLS protocols not
supported by libESMTP. This may done using [`smtp_starttls_set_ctx()`][1] which
must be called before [`smtp_start_session()`][2].

Note that when using an application supplied SSL_CTX, libESMTP does not load CA
certificate chains or a default client certificate, however, it still checks
for host-specific client certificates as described above.


[1]: smtp-tls.html#c.smtp_starttls_set_ctx
[2]: smtp-api.html#c.smtp_start_session
[3]: https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
[4]: https://www.openssl.org/docs/man1.1.1/man1/c_rehash.html
