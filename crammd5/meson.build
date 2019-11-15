sasl_crammd5_sources = [
  'client-crammd5.c',
  'hmacmd5.c',
  'hmacmd5.h',
]

if conf.get('USE_TLS')
  sasl_crammd5_sources += [
    'md5.c',
    'md5.h',
  ]
endif

sasl_crammd5 = shared_module('sasl-crammd5', sasl_crammd5_sources,
			     include_directories: [ include_dir, ],
			     install : true,
			     install_dir: auth_plugin_dir)
clients += sasl_crammd5
