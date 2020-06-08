if get_option('hotdoc')
    hotdoc = import('hotdoc')

    hotdoc.generate_doc('libesmtp',
			project_version : libesmtp_version,
			sitemap: 'sitemap.txt',
			c_index: 'index.md',
			index: 'index.md',
			c_sources : sources,
			languages : ['c'],
			verbose : true,
			install : true)
endif

if get_option('gtkdoc')
    docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

    gnome.gtkdoc('libesmtp',
	dependencies : [declare_dependency(link_with : lib)],
	main_xml : 'libesmtp-docs.xml',
	src_dir : [meson.source_root(), meson.build_root()],
	mkdb_args: [
	  '--default-includes=libesmtp.h',
	],
	fixxref_args: [
	    '--html-dir=@0@'.format(docpath),
	],
	install : true
    )
endif
