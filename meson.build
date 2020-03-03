project('libESMTP', 'c', version : '1.0.7', default_options: ['c_std=c11'])
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

libesmtp_version = meson.project_version()
so_version = '7.6.2'

auth_plugin_dir = get_option('libdir') / 'esmtp-plugins-' + so_version


################################################################################
# legacy defines
################################################################################
cflags = [
  '-D_POSIX_C_SOURCE=200809L',
  '-DHAVE_CONFIG_H',
  '-DHAVE_CONFIG',
]
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

################################################################################
# dependencies
################################################################################
dldep = cc.find_library('dl', required : false)
have_dl = dldep.found()
if not dldep.found()
    dldep = cc.find_library('ltdl')
endif
ssldep = dependency('openssl', required : get_option('tls'))
threaddep = dependency('threads', required : get_option('pthreads'))
lwresdep = cc.find_library('lwres', required : get_option('lwres'))

deps = [
  dldep,
  ssldep,
  threaddep,
  lwresdep,
]

################################################################################
# Function availability
################################################################################

################################################################################
# configuration
# XXX WARNING use of #if SYMBOL vs #ifdef SYMBOL somewhat haphazard
################################################################################
conf = configuration_data()
conf.set('SIZEOF_UNSIGNED_INT', cc.sizeof('unsigned int'))
conf.set('SIZEOF_UNSIGNED_LONG', cc.sizeof('unsigned long'))
conf.set('SIZEOF_UNSIGNED_SHORT', cc.sizeof('unsigned short'))

conf.set('AUTH_ID_HACK', true)
conf.set('USE_CHUNKING', true)
conf.set('USE_ETRN', false)
conf.set('USE_IPV6', true)
conf.set('USE_PTHREADS', threaddep.found())
conf.set('USE_SASL', true)
conf.set('USE_TLS', ssldep.found())
conf.set('USE_XUSR', true)

conf.set_quoted('AUTHPLUGINDIR', get_option('prefix') / auth_plugin_dir)
conf.set_quoted('SO_VERSION', so_version)
conf.set_quoted('VERSION', libesmtp_version)

conf.set('HAVE_DLSYM', have_dl.to_int(), description : 'POSIX.1-2001')
conf.set('HAVE_GETADDRINFO', 1, description : 'POSIX.1-2001, POSIX.1-2008.')
conf.set('HAVE_GETHOSTNAME', 1, description : 'POSIX.1-2001, POSIX.1-2008.')
conf.set('HAVE_GETTIMEOFDAY', true, description : 'POSIX.1-2001, obsolete POSIX.1-2008.')
conf.set('HAVE_LIBCRYPTO', ssldep.found().to_int())
conf.set('HAVE_LOCALTIME_R', 1, description : 'SUSV2')
conf.set('HAVE_LWRES_NETDB_H', lwresdep.found().to_int())
conf.set('HAVE_STRERROR_R', 1)
conf.set('HAVE_WORKING_STRERROR_R', 0)
conf.set('HAVE_STRUCT_TM_TM_ZONE', 0)
conf.set('HAVE_UNAME', 1)

# missing.h - probably all ok to remove
conf.set('HAVE_STRDUP', true)
conf.set('HAVE_STRNCASECMP', true)
conf.set('HAVE_STRCASECMP', true)
conf.set('HAVE_MEMRCHR', false)
conf.set('HAVE_SNPRINTF', true)
conf.set('HAVE_VSNPRINTF', true)

# snprintf.c - probably ok to remove
conf.set('HAVE_STDARG_H', true)
conf.set('HAVE_STDARGS', false)
conf.set('HAVE_VARARGS_H', false)

# getaddrinfo.c, gethostbyname.c
# getaddrinfo() implementation over gethostbyname()
conf.set('HAVE_GETHOSTBYNAME_R', 0)
conf.set('HAVE_GETIPNODEBYNAME', 0, description : 'DEPRECATED')

configure_file(output : 'config.h', configuration : conf)

################################################################################
# libesmtp.spec.in
################################################################################

spec = configuration_data()
spec.set('PACKAGE', meson.project_name())
spec.set('VERSION', libesmtp_version)
spec.set('BINDIR', get_option('bindir'))
spec.set('LIBDIR', get_option('libdir'))
spec.set('PLUGINDIR', auth_plugin_dir)
spec.set('URL', 'https://www.github.com/briancs/libESMTP/web/')
spec.set('EMAIL', 'Brian Stafford <brian.stafford60@gmail.com>')

if ssldep.found()
  spec.set('RPM_REQUIRES', 'Requires:')
  spec.set('RPM_BUILDREQUIRES', 'BuildRequires:')
  spec.set('RPM_OPENSSL', 'openssl >= %{openssl}')
  spec.set('RPM_OPENSSLDEVEL', 'openssl-devel >= %{openssl}')
endif

configure_file(input : 'libesmtp.spec.in',
	       output : 'libesmtp.spec',
	       configuration : spec)

################################################################################
# targets
################################################################################
sources = [
  'api.h',
  'auth-client.c',
  'base64.c',
  'base64.h',
  'concatenate.c',
  'concatenate.h',
  'errors.c',
  'headers.c',
  'headers.h',
  'htable.c',
  'htable.h',
  'libesmtp.h',
  'libesmtp-private.h',
  'memrchr.c',
  'message-callbacks.c',
  'message-source.c',
  'message-source.h',
  'missing.h',
  'protocol.c',
  'protocol.h',
  'protocol-states.h',
  'rfc2822date.c',
  'rfc2822date.h',
  'siobuf.c',
  'siobuf.h',
  'smtp-api.c',
  'smtp-auth.c',
  'smtp-bdat.c',
  'smtp-etrn.c',
  'smtp-tls.c',
  'tokens.c',
  'tokens.h'
]

lib = library('esmtp', sources,
	      soversion : so_version,
	      dependencies : deps,
	      install : true)

################################################################################
# SASL client modules
################################################################################
clients = []
include_dir = include_directories('.')
subdir('login')
subdir('plain')
subdir('crammd5')
if ssldep.found()
  subdir('ntlm')
endif

################################################################################
# Misc installation
################################################################################
install_headers(['libesmtp.h'])
pkg.generate(lib, filebase : 'libesmtp-1.0', description : 'SMTP client library')

