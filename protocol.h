#ifndef _protocol_h
#define _protocol_h
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

/* Create some symbolic state numbers */
enum states
  {
#define S(x)		S_##x,
#include "protocol-states.h"
    S__swallow_comma		/* required for strict ANSI compiles */
  };

/* Forward declare protocol command/response handlers */
#define S(x)		void cmd_##x (siobuf_t conn, smtp_session_t session); \
			void rsp_##x (siobuf_t conn, smtp_session_t session);
#include "protocol-states.h"

int read_smtp_response (siobuf_t conn, smtp_session_t session,
			struct smtp_status *status,
			int (*cb) (smtp_session_t, char *));

#endif
