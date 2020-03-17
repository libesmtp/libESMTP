# Download

LibESMTP is considered stable.  Source code is available from
[GitHub](https://github.com/) at
[https://github.com/briancs60/libESMTP](https://github.com/briancs60/libESMTP).
Clone the repository using
``` sh
$ git clone https://github.com/briancs60/libESMTP.git
```

## Dependencies

### OpenSSL
[OpenSSL v1.1.0](https://www.openssl.org/) or later is required to build the
SMTP STARTTLS extension and, if deprecated features are enabled, to build the
SASL NTLM authentication module.  If you have no need for either of these
features, you do not need OpenSSL.

Please note that OpenSSL is distributed under the
[Apache License v2](https://www.openssl.org/source/apache-license-2.0.txt)
If this is problematic for you,  LibESMTP can be built without support for
features which depend on OpenSSL and the resulting binary will be pure LGPL.

### dlopen()
libESMTP requires `dlopen()` to dynamically load plugin modules.

### getaddrinfo()
You will need a modern resolver library providing the getaddrinfo API.
This should be true of all contemporary systems.

Refer to INSTALL.md in the distribution for further details.

## Configuration
LibESMTP uses the [Meson build system](https://mesonbuild.com/Getting-meson.html)
Refer to the Meson manual for standard configuration options. `INSTALL.md`
describes options specific to libESMTP.
