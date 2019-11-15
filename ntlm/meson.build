sasl_ntlm_sources = [
  'client-ntlm.c',
  'ntlmdes.c',
  'ntlm.h',
  'ntlmstruct.c',
]

sasl_ntlm = shared_module('sasl-ntlm', sasl_ntlm_sources,
			  include_directories: [ include_dir, ],
			  install : true,
			  install_dir: auth_plugin_dir)
clients += sasl_ntlm
