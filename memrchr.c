/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2004  Brian Stafford  <brian@stafford.uklinux.net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#undef HAVE_MEMRCHR
#undef _GNU_SOURCE
#undef _ISOC9X_SOURCE
#undef _OSF_SOURCE
#undef _XOPEN_SOURCE
#undef __EXTENSIONS__
#include <missing.h>

void *
memrchr (const void *a, int c, size_t len)
{
  const unsigned char *p = a;
  
  for (p += len - 1; (const void *) p >= a; p--)
    if (*p == c)
      return (void *) p;
  return (void *) 0;
}
