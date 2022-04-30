API Reference
=============

.. admonition:: This document describes the libESMTP v1.1 programming interface.
    :class: attention

    The latest version of this document is always available at
    `libesmtp.github.io <https://libesmtp.github.io>`_.

The libESMTP API is intended for use as an SMTP client within a Mail User
Agent (MUA) or other program that wishes to submit mail to a preconfigured
Message Submission Agent (MSA), sometimes known as a *smart host*.

The documentation is believed to be accurate however:

  Much of it is apocryphal or, at the very least, wildly inaccurate.
  However, where it is inaccurate, it is *definitively* inaccurate.

  -- Douglas Adams, *The Hitch Hikers' Guide to the Galaxy*

.. toctree::
   :maxdepth: 2

   introduction
   programflow
   certificates
   _kdoc/libesmtp
   _kdoc/smtp-api
   _kdoc/smtp-tls
   _kdoc/smtp-auth
   _kdoc/auth-client
   _kdoc/message-callbacks
   _kdoc/headers
   _kdoc/events
   _kdoc/tlsevents
   _kdoc/smtp-etrn
   _kdoc/errors
   licence
   /genindex

