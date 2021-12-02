/*
 *  This file is part of libESMTP, a library for submission of RFC 5322
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 5321.
 *
 *  Copyright (C) 2001-2021  Brian Stafford  <brian.stafford60@gmail.com>
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

#include <config.h>
#include <stdio.h>
#include <time.h>

#include <missing.h> /* declarations for missing library functions */

#include "rfc2822date.h"

#if HAVE_STRUCT_TM_TM_ZONE
#  define gmt_offset(tm, td)  (tm)->tm_gmtoff
#elif HAVE_TIMEZONE
#  define gmt_offset(tm, td)  timezone
#else
/* Calculate seconds in the current day from the struct tm.  Leap seconds are
   ignored, tm must be normalised and the time zone is ignored.  If called with
   a struct tm filled in by localtime() the result will be wrong by the number
   of seconds the current timezone is offset from GMT!  */
static int
gmt_offset (struct tm *tm, time_t *timedate)
{
  long gmtoff;

  gmtoff = (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec;
  gmtoff -= *timedate % 86400L;
  return gmtoff;
}
#endif

#if !HAVE_LOCALTIME_R
static struct tm *
localtime_r (const time_t *tmbuf, struct tm *timedate __attribute__ ((unused)))
{
  return localtime (tmbuf);
}
#endif

static const char *days[] =
  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", };
static const char *months[] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", };

char *
rfc2822date (char buf[], size_t buflen, time_t *timedate)
{
  struct tm tmbuf, *tm;
  int dir, minutes;

  tm = localtime_r (timedate, &tmbuf);

  /* It's not clear from the description of tm_isdst whether a value less than
     zero means that whether daylight saving (summer) time is in effect could
     not be determined or whether the offset from UTC is unknown.  Some
     implementations of strftime(3) assume the latter so we do the same */
  if (tm->tm_isdst >= 0)
    {
      minutes = gmt_offset (tm, timedate) / 60;
      dir = (minutes >= 0) ? '+' : '-';
      if (minutes < 0)
	minutes = -minutes;
    }
  else
    {
      /* RFC 5322 - zone = "-0000" means timezone could not be determined */
      dir = '-';
      minutes = 0;
    }
  snprintf (buf, buflen, "%s, %d %s %d %02d:%02d:%02d %c%02d%02d",
	    days[tm->tm_wday],
	    tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
	    tm->tm_hour, tm->tm_min, tm->tm_sec,
	    dir, minutes / 60, minutes % 60);
  return buf;
}
