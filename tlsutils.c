/*
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2001-2020  Brian Stafford  <brian.stafford60@gmail.com>
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
#include <ctype.h>
#include <string.h>
#include "tlsutils.h"
#include "missing.h"

static int
match_component (const char *dom, const char *edom,
		 const char *ref, const char *eref)
{
  /* If ref is the single character '*' then accept this as a wildcard
     matching any valid domainname component, i.e. characters from the
     range A-Z, a-z, 0-9, - or _
     NB this is more restrictive than RFC 2818 which allows multiple
     wildcard characters in the component pattern */
  if (eref == ref + 1 && *ref == '*')
    while (dom < edom)
      {
	if (!(isalnum (*dom) || *dom == '-'))
	  return 0;
	dom++;
      }
  else
    {
      while (dom < edom && ref < eref)
	{
	  /* check for valid domainname character */
	  if (!(isalnum (*dom) || *dom == '-'))
	    return 0;
	  /* compare the domain name case-insensitively */
	  if (!(*dom == *ref || tolower (*dom) == tolower (*ref)))
	    return 0;
	  ref++, dom++;
	}
      if (dom < edom || ref < eref)
	return 0;
    }
  return 1;
}

/* Perform a domain name comparison where the reference may contain
   wildcards.  This implements the comparison from RFC 2818.
   Each component of the domain name is matched separately, working from
   right to left.
 */
int
match_domain (const char *domain, const char *reference)
{
  const char *dom, *edom, *ref, *eref;

  eref = strchr (reference, '\0');
  edom = strchr (domain, '\0');
  while (eref > reference && edom > domain)
    {
      /* Find the rightmost component of the reference. */
      ref = memrchr (reference, '.', eref - reference - 1);
      if (ref != NULL)
	ref++;
      else
	ref = reference;

      /* Find the rightmost component of the domain name. */
      dom = memrchr (domain, '.', edom - domain - 1);
      if (dom != NULL)
	dom++;
      else
	dom = domain;

      if (!match_component (dom, edom, ref, eref))
	return 0;
      edom = dom - 1;
      eref = ref - 1;
    }
  return eref < reference && edom < domain;
}
