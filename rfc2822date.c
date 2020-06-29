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

#include <config.h>

#include <stdio.h>
#include <sys/types.h>

#include <missing.h> /* declarations for missing library functions */

#if TM_IN_SYS_TIME
# include <sys/time.h>
# if TIME_WITH_SYS_TIME
#  include <time.h>
# endif
#else
# include <time.h>
#endif

#include "rfc2822date.h"

#if HAVE_STRFTIME

char *
rfc2822date (char buf[], size_t buflen, time_t *timedate)
{
#if HAVE_LOCALTIME_R
  struct tm tmbuf, *tm;

  tm = localtime_r (timedate, &tmbuf);
#else
  struct tm *tm;

  tm = localtime (timedate);
#endif
  strftime (buf, buflen, "%a, %d %b %Y %T %z", tm);
  return buf;
}

#else

#if !HAVE_STRUCT_TM_TM_ZONE
/* Calculate seconds since 1970 from the struct tm.  Leap seconds are
   ignored, tm must be normalised and the time zone is ignored.
   If called with a struct tm filled in by localtime() the result
   will be wrong by the number of seconds the current timezone is
   offset from GMT!  */

#define EPOCH 1970

static time_t
libesmtp_mktime (struct tm *tm)
{
  time_t when;
  int day, year;
  static const int days[] =
    {
      0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
    };

  year = tm->tm_year + 1900;
  day = days[tm->tm_mon] + tm->tm_mday - 1;

  /* adjust for leap years paying attention to January and February */
  day += (year - (EPOCH - (EPOCH % 4))) / 4;
  day -= (year - (EPOCH - (EPOCH % 100))) / 100;
  day += (year - (EPOCH - (EPOCH % 400))) / 400;
  if (tm->tm_mon < 2 && (year % 4 == 0) && (year % 100 != 0 || year / 400 == 0))
    day -= 1;

  when = ((year - EPOCH) * 365 + day) * 24 + tm->tm_hour;
  when = (when * 60 + tm->tm_min) * 60 + tm->tm_sec;
  return when;
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
  struct tm *tm;
#if HAVE_LOCALTIME_R
  struct tm tmbuf;
#endif
  int dir, minutes;
#if !HAVE_STRUCT_TM_TM_ZONE
  time_t gmtoff;
#endif

#if HAVE_LOCALTIME_R
  tm = localtime_r (timedate, &tmbuf);
#else
  tm = localtime (timedate);
#endif
#if HAVE_STRUCT_TM_TM_ZONE
  minutes = tm->tm_gmtoff / 60;
#else
  gmtoff = libesmtp_mktime (tm);
  gmtoff -= *timedate;
  minutes = gmtoff / 60;
#endif

  dir = (minutes > 0) ? '+' : '-';
  if (minutes < 0)
    minutes = -minutes;
  snprintf (buf, buflen, "%s, %d %s %d %02d:%02d:%02d %c%02d%02d",
	    days[tm->tm_wday],
	    tm->tm_mday, months[tm->tm_mon], tm->tm_year + 1900,
	    tm->tm_hour, tm->tm_min, tm->tm_sec,
	    dir, minutes / 60, minutes % 60);
  return buf;
}

#endif
