mail_file_sources = [
  'mail-file.c',
]

if ssldep.found()

mail_file = executable('mail-file', mail_file_sources,
		       dependencies : ssldep,
		       link_with : lib,
		       include_directories: [ include_dir, ])

endif
