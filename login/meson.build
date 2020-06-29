sasl_login_sources = [
  'client-login.c'
]

sasl_login = shared_module('login', sasl_login_sources,
			   name_prefix : 'sasl-',
			   include_directories: [ include_dir, ],
			   install : true,
			   install_dir: auth_plugin_dir)
clients += sasl_login
