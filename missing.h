#ifndef _missing_h
#define _missing_h
/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2002-2004  Brian Stafford  <brian@stafford.uklinux.net>
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

/* Collect together some declarations that might not be present on
   some systems */

#include <stddef.h>		/* for size_t */

#ifndef HAVE_STRDUP
char *strdup (const char *s1);
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp (const char *a, const char *b);
#endif

#ifndef HAVE_STRNCASECMP
int strncasecmp (const char *a, const char *b, size_t len);
#endif

#ifndef HAVE_MEMRCHR
void *memrchr (const void *a, int c, size_t len);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy (char *dest, const char *src, size_t dest_size);
#endif

#endif
