project('libESMTP', 'c', version : '1.0.7', default_options: ['c_std=c11'])
email = 'Brian Stafford <brian.stafford60@gmail.com>'
website = 'https://www.github.com/briancs/libESMTP/web/'

pkg = import('pkgconfig')
cc = meson.get_compiler('c')

libesmtp_version = meson.project_version()
libesmtp_so_version = '7.6.2'

auth_plugin_dir = get_option('libdir') / 'esmtp-plugins-' + libesmtp_so_version

################################################################################
# legacy defines
################################################################################
cflags = [
  '-D_POSIX_C_SOURCE=200809L',
]

cflags_warnings = [
    '-Walloc-zero',
    '-Wbad-function-cast',
    '-Wcast-align',
    '-Wcast-qual',
    '-Wchar-subscripts',
    '-Wdeclaration-after-statement',
    '-Wduplicated-branches',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wpacked',
    '-Wpointer-arith',
    '-Wredundant-decls',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wwrite-strings',
]

if get_option('warning_level').to_int() >= 2
    cflags += cflags_warnings
endif
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

################################################################################
# dependencies
################################################################################
dldep = cc.find_library('dl', required : false)
have_dl = dldep.found()
if not dldep.found()
    dldep = cc.find_library('ltdl')
endif
ssldep = dependency('openssl', version : '>=1.1.0', required : get_option('tls'))
threaddep = dependency('threads', required : get_option('pthreads'))

#XXX add test for libbind9.so
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

# Solaris 8 strlcpy() return value does not correspond with BSD version
# libESMTP does not use the return value so this should be sufficient
have_strlcpy = cc.has_function('strlcpy') or meson.get_cross_property('have_strlcpy', false)

################################################################################
# configuration
# XXX WARNING use of #if SYMBOL vs #ifdef SYMBOL somewhat haphazard
################################################################################
conf = configuration_data()
conf.set('SIZEOF_UNSIGNED_INT', cc.sizeof('unsigned int'))
conf.set('SIZEOF_UNSIGNED_LONG', cc.sizeof('unsigned long'))
conf.set('SIZEOF_UNSIGNED_SHORT', cc.sizeof('unsigned short'))

conf.set('AUTH_ID_HACK', true)
conf.set('USE_CHUNKING', get_option('bdat'))
conf.set('USE_ETRN', get_option('etrn'))
conf.set('USE_PTHREADS', threaddep.found())
conf.set('USE_TLS', ssldep.found())
conf.set('USE_XUSR', get_option('xusr'))

conf.set_quoted('AUTHPLUGINDIR', get_option('prefix') / auth_plugin_dir)
conf.set_quoted('SO_VERSION', libesmtp_so_version)
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
conf.set('HAVE_STRFTIME', 1)

# endianness
conf.set('WORDS_BIGENDIAN', host_machine.endian() == 'big')

# missing.h
conf.set('HAVE_STRDUP', true)
conf.set('HAVE_STRNCASECMP', true)
conf.set('HAVE_STRCASECMP', true)
conf.set('HAVE_MEMRCHR', false)
conf.set('HAVE_STRLCPY', have_strlcpy)

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
spec.set('URL', website)
spec.set('EMAIL', email)

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
  'message-callbacks.c',
  'message-source.c',
  'message-source.h',
  'missing.c',
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
if ssldep.found()
  sources += [ 'tlsutils.h', 'tlsutils.c' ]
endif

mapfile = 'libesmtp.map'
vflag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)

lib = library('esmtp', sources,
	      link_args : vflag, link_depends : mapfile,
	      soversion : libesmtp_so_version,
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
install_headers(['libesmtp.h', 'auth-client.h'])
pkg.generate(lib, filebase : 'libesmtp-1.0', description : 'SMTP client library')

