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

/* To add a new command or state to the SMTP protocol engine, give it
   a name in this file and write the functions to handle the state.  */

S (greeting)
S (ehlo)
S (helo)
#ifdef USE_TLS
S (starttls)
#endif
S (auth)
S (auth2)
#ifdef USE_ETRN
S (etrn)
#endif
#ifdef USE_XUSR
S (xusr)
#endif
S (mail)
S (rcpt)
S (data)
S (data2)
#ifdef USE_CHUNKING
S (bdat)
S (bdat2)
#endif
S (rset)
S (quit)

#undef S
