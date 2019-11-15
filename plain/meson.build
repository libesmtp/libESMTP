sasl_plain_sources = [
  'client-plain.c'
]

sasl_plain = shared_module('sasl-plain', sasl_plain_sources,
			   include_directories: [ include_dir, ],
			   install : true,
			   install_dir: auth_plugin_dir)
clients += sasl_plain
