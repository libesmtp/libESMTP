#ifndef _htable
#define _htable
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

struct h_node;

void *h_insert (struct h_node **table,
		const char *name, int namelen, size_t size);
void h_remove (struct h_node **table, void *data);
void *h_search (struct h_node **table, const char *name, int namelen);
void h_enumerate (struct h_node **table,
		 void (*cb) (const char *name, void *data, void *arg),
		 void *arg);
struct h_node **h_create (void);
void h_destroy (struct h_node **table,
		void (*cb) (const char *name, void *data, void *arg),
		void *arg);

#endif
