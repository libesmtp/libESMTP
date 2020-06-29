.. libESMTP documentation master file, created by
   sphinx-quickstart on Wed Jun 24 17:54:06 2020.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

========
libESMTP
========

.. note:: This is the official libESMTP repository and documentation,
    created and maintained by the original libESMTP author.  The author does
    not endorse other repositories which may contain forks of libESMTP.

    It is requested that any modifications are branched and merged from
    the official repository at https://github.com/libesmtp/libESMTP .

A Library for Posting Electronic Mail
=====================================

**libESMTP** is an SMTP client which manages submission of electronic mail via
a preconfigured Mail Transport Agent (MTA) such as
`Exim <https://www.exim.org/>`_ or `Postfix <http://www.postfix.org/>`_.

libESMTP offloads the developer of the need to handle the complexity of
negotiating the SMTP protocol by providing a simple API. Additionally
libESMTP transparently handles many SMTP extensions including authentication.

.. toctree::
   :maxdepth: 1

   libesmtp
   download
   bugreport
   faq
   rationale
   retrospective
   reference

.. toctree::
   :hidden:

   ChangeLog

Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`

Copyright & Licence
-------------------

libESMTP is copyright © 2001-2020 Brian Stafford, licensed under the
`GNU Lesser General Public License <https://www.gnu.org/copyleft/lesser.html)>`_.

The example programs are licensed
`GNU General Public Licence <https://www.gnu.org/copyleft/gpl.html>`_.
