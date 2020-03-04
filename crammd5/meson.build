sasl_crammd5_sources = [
  'client-crammd5.c',
  'hmacmd5.c',
  'hmacmd5.h',
]

crammd5_deps = [ ]

if conf.get('USE_TLS')
  crammd5_deps += ssldep
else
  sasl_crammd5_sources += [
    'md5.c',
    'md5.h',
  ]
endif

sasl_crammd5 = shared_module('crammd5', sasl_crammd5_sources,
			     name_prefix : 'sasl-',
			     dependencies : crammd5_deps,
			     include_directories: [ include_dir, ],
			     install : true,
			     install_dir: auth_plugin_dir)
clients += sasl_crammd5
