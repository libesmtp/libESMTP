/* Turn off the GCC specific __attribute__ keyword */
#if !defined (__GNUC__) || __GNUC__ < 2
# define __attribute__(x)
#endif

@TOP@

@BOTTOM@

#ifndef HAVE_STRCASECMP
int strcasecmp (const char *a, const char *b);
#endif
#ifndef HAVE_STRNCASECMP
int strncasecmp (const char *a, const char *b, size_t len);
#endif
#ifndef HAVE_STRNDUP
char *strndup (const char *string, size_t length);
#endif
