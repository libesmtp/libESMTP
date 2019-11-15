sasl_login_sources = [
  'client-login.c'
]

sasl_login = shared_module('sasl-login', sasl_login_sources,
			   include_directories: [ include_dir, ],
			   install : true,
			   install_dir: auth_plugin_dir)
clients += sasl_login
