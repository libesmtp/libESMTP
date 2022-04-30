project('libESMTP', 'c', version : '1.1.1', default_options: ['c_std=c11'])
email = 'Brian Stafford <brian.stafford60@gmail.com>'
website = 'https://libesmtp.github.io/'

pkg = import('pkgconfig')
cc = meson.get_compiler('c')

libesmtp_version = meson.project_version()

# As a reference point, v1.0.6, the last widely released version, had the
# libtool style current:revision:age = 7:6:1 where:
#    current = API version
#    revision = bug fix
#    age = number of backward compatible APIs
libesmtp_cra = '8:1:2'

# The .so version number is calculated as C:R:A => .(C-A).A.R
cra = []
foreach str : libesmtp_cra.split(':')
    cra += str.to_int()
endforeach
libesmtp_so_version = '@0@.@1@.@2@'.format(cra[0]-cra[2], cra[2], cra[1])

auth_plugin_dir = get_option('libdir') / 'esmtp-plugins-' + libesmtp_so_version

################################################################################
# legacy defines
################################################################################
cflags = []

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
if get_option('warning_level').to_int() >= 3
    cflags += ['-Werror']
endif
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

################################################################################
# dependencies
################################################################################
dldep = cc.find_library('dl')
ssldep = dependency('openssl', version : '>=1.1.1', required : get_option('tls'))
ntlmdep = dependency('openssl', version : '>=1.1.1', required : get_option('ntlm'))
threaddep = dependency('threads', required : get_option('pthreads'))

#XXX add test for libbind9.so
lwresdep = cc.find_library('lwres', required : get_option('lwres'))

deps = [
  dldep,
  ssldep,
  ntlmdep,
  threaddep,
  lwresdep,
]

################################################################################
# Function availability
################################################################################

# Solaris 8 strlcpy() return value does not correspond with BSD version
# libESMTP does not use the return value so this should be sufficient
have_strlcpy = cc.has_function('strlcpy')
have_strdup = cc.has_function('strdup')
have_strncasecmp = cc.has_function('strncasecmp')
have_strcasecmp = cc.has_function('strcasecmp')
have_memrchr = cc.has_header_symbol('string.h', 'memrchr',
                                    args: '-D_GNU_SOURCE')
have_localtime_r = cc.has_function('localtime_r')
have_gmtoff = cc.has_member('struct tm', 'tm_gmtoff',
                            args: '-D_GNU_SOURCE',
                            prefix: '#include <time.h>')
have_timezone = cc.has_header_symbol('time.h', 'timezone',
                                     args: '-D_XOPEN_SOURCE=700')



################################################################################
# configuration
# XXX WARNING use of #if SYMBOL vs #ifdef SYMBOL somewhat haphazard
################################################################################
conf = configuration_data()
gnu_source = have_memrchr or have_gmtoff
xopen_source = have_timezone
if gnu_source
    conf.set('_GNU_SOURCE', true)
elif xopen_source
    conf.set('_XOPEN_SOURCE', 700)
else
    conf.set('_POSIX_C_SOURCE', '200809L')
endif
conf.set('SIZEOF_UNSIGNED_INT', cc.sizeof('unsigned int'))
conf.set('SIZEOF_UNSIGNED_LONG', cc.sizeof('unsigned long'))
conf.set('SIZEOF_UNSIGNED_SHORT', cc.sizeof('unsigned short'))

conf.set('USE_CHUNKING', get_option('bdat'))
conf.set('USE_ETRN', get_option('etrn'))
conf.set('USE_PTHREADS', threaddep.found())
conf.set('USE_TLS', ssldep.found())
conf.set('USE_XDG_DIRS', get_option('xdg'))
conf.set('USE_XUSR', get_option('xusr'))

conf.set_quoted('AUTHPLUGINDIR', get_option('prefix') / auth_plugin_dir)
conf.set_quoted('LT_VERSION', libesmtp_cra)
conf.set_quoted('SO_VERSION', libesmtp_so_version)
conf.set_quoted('VERSION', libesmtp_version)

conf.set('HAVE_GETHOSTNAME', 1, description : 'POSIX.1-2001, POSIX.1-2008.')
conf.set('HAVE_GETTIMEOFDAY', true, description : 'POSIX.1-2001, obsolete POSIX.1-2008.')
conf.set('HAVE_LIBCRYPTO', ssldep.found().to_int())
conf.set('HAVE_LWRES_NETDB_H', lwresdep.found().to_int())
conf.set10('HAVE_STRERROR_R', not gnu_source)
conf.set10('HAVE_GNU_STRERROR_R', gnu_source)
conf.set('HAVE_UNAME', 1)

conf.set10('HAVE_LOCALTIME_R', have_localtime_r)
conf.set10('HAVE_TIMEZONE', have_timezone)
conf.set10('HAVE_STRUCT_TM_TM_ZONE', have_gmtoff)

conf.set('LIBESMTP_ENABLE_DEPRECATED_SYMBOLS', true)

# endianness
conf.set('WORDS_BIGENDIAN', host_machine.endian() == 'big')

# missing.h
conf.set('HAVE_STRDUP', have_strdup)
conf.set('HAVE_STRNCASECMP', have_strncasecmp)
conf.set('HAVE_STRCASECMP', have_strcasecmp)
conf.set('HAVE_MEMRCHR', have_memrchr)
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
  'auth-client.h',
  'auth-plugin.h',
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
  'tlsutils.c',
  'tlsutils.h',
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
	      version : libesmtp_so_version,
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
if ntlmdep.found()
  if cc.has_header('openssl/md4.h') and cc.has_function('MD4_Init', dependencies : ntlmdep)
    subdir('ntlm')
  else
    error('MD4 is not supported in current openssl, unable to build NTLM plugin')
  endif
endif

################################################################################
# libESMTP example/demo program
################################################################################
subdir('examples')

################################################################################
# Misc installation
################################################################################
install_headers(['libesmtp.h', 'auth-client.h'])
pkg.generate(lib, filebase : 'libesmtp-1.0', description : 'SMTP client library')


summary({'current:revision:age': libesmtp_cra,
         'so version': libesmtp_so_version,
         'prefix': get_option('prefix'),
         'libdir': get_option('libdir'),
	 'threads': threaddep.found(),
	 'lwres': lwresdep.found(),
	 'AUTH modules': get_option('prefix') / auth_plugin_dir,
	 'Legacy file layout': not get_option('xdg'),
	 'STARTTLS': ssldep.found(),
	 'CHUNKING': get_option('bdat'),
	 'ETRN': get_option('etrn'),
	 'XUSR': get_option('xusr'),
	 'NTLM': ntlmdep.found()})
