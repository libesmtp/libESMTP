/*
 *  This file is part of libESMTP, a library for submission of RFC 822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 821.
 *
 *  Copyright (C) 2001  Brian Stafford  <brian@stafford.uklinux.net>
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

#include <stdio.h>
#include <sys/types.h>

#if TM_IN_SYS_TIME
# include <sys/time.h>
# if TIME_WITH_SYS_TIME
#  include <time.h>
# endif
#else
# include <time.h>
#endif

#include "rfc822date.h"

static const char *days[] =
  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", };
static const char *months[] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", };

char *
rfc822date (char buf[], size_t buflen, time_t *timedate)
{
  struct tm *tm;
#if defined (HAVE_LOCALTIME_R) || defined (HAVE_GMTIME_R)
  struct tm tmbuf;
#endif
  int dir, minutes;

#if defined (HAVE_STRUCT_TM_TM_ZONE)
# if defined (HAVE_LOCALTIME_R)
  tm = localtime_r (timedate, &tmbuf);
# else
  tm = localtime (timedate);
# endif
  minutes = -tm->tm_gmtoff / 60;
  dir = (minutes > 0) ? '-' : '+';
#else
  /* Just cop out.  Use Greenwich Time and set the offset to +0000 */
# if defined (HAVE_GMTIME_R)
  tm = gmtime_r (timedate, &tmbuf);
# else
  tm = gmtime (timedate);
# endif
  minutes = 0;
  dir = '+';
#endif

  if (minutes < 0)
    minutes = -minutes;
  snprintf (buf, buflen, "%s, %d %s %d %02d:%02d:%02d %c%02d%02d",
	    days[tm->tm_wday],
	    tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
	    tm->tm_hour, tm->tm_min, tm->tm_sec,
	    dir, minutes / 60, minutes % 60);
  return buf;
}
