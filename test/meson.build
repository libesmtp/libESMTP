
dep = dependency('check')

base64 = executable('base64', 'test-base64.c',
		    dependencies : [ dep],
		    include_directories : [include_dir,])
test('base64', base64)

concatenate = executable('concatenate', 'test-concatenate.c',
		    dependencies : [ dep],
		    include_directories : [include_dir,])
test('concatenate', concatenate)

missing = executable('missing', 'test-missing.c',
		    dependencies : [ dep],
		    include_directories : [include_dir,])
test('missing', missing)

tlsutils = executable('tlsutils', 'test-tlsutils.c',
		    dependencies : [ dep],
		    include_directories : [include_dir,])
test('tlsutils', tlsutils)
