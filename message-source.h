#ifndef _message_source_h
#define _message_source_h
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

/*  Functions to read lines or blocks of text from the message source.
    These functions allow the library to interface to the application
    using a callback function.  This is intended to allow the application
    maximum flexibility in managing its message storage.  */

typedef struct msg_source *msg_source_t;

msg_source_t msg_source_create (void);
void msg_source_destroy (msg_source_t source);
void msg_source_set_cb (msg_source_t source,
			const char *(*cb) (void **ctx, int *len, void *arg),
			void *arg);
void msg_rewind (msg_source_t source);
const char *msg_gets (msg_source_t source, int *len, int concatenate);
int msg_nextc (msg_source_t source);
const char *msg_getb (msg_source_t source, int *len);

#endif
