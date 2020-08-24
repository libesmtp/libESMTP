#ifndef _ntlm_h
#define _ntlm_h
/*
 *  Authentication module for the Micr$oft NTLM mechanism.
 *
 *  This file is part of libESMTP, a library for submission of RFC 2822
 *  formatted electronic mail messages using the SMTP protocol described
 *  in RFC 2821.
 *
 *  Copyright (C) 2002  Brian Stafford  <brian@stafford.uklinux.net>
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

int lm_uccpy (char *dst, size_t dstlen, const char *src);
unsigned char *nt_unicode (const char *string, size_t len);

void lm_hash_password (unsigned char *hash, const char *pass);
void nt_hash_password (unsigned char *hash, const char *pass);

void ntlm_responses (unsigned char *lm_resp, unsigned char *nt_resp,
		     const unsigned char *challenge, const char *secret);

size_t ntlm_build_type_1 (char *buf, size_t buflen, unsigned int flags,
			  const char *domain, const char *workstation);
size_t ntlm_build_type_2 (char *buf, size_t buflen, unsigned int flags,
			  const unsigned char *nonce, const char *domain);
size_t ntlm_build_type_3 (char *buf, size_t buflen,
			  unsigned int flags,
			  const unsigned char *lm_resp,
			  const unsigned char *nt_resp,
			  const char *domain, const char *user,
			  const char *workstation);

size_t ntlm_parse_type_2 (const char *buf, size_t buflen, unsigned int *flags,
		          unsigned char *nonce, char **domain);

#define TYPE1_FLAGS	0x8202u

#endif
