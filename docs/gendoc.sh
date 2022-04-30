#!/bin/sh

SRC=..
DST=_kdoc

SOURCES="libesmtp.h message-callbacks.c
smtp-api.c  smtp-auth.c  smtp-etrn.c  smtp-tls.c errors.c
auth-client.c headers.c
"

mkdir -p $DST
rm $DST/*.rst
for file in $SOURCES
do
	base=${file##*/}
	kernel-doc -rst $SRC/${file} > $DST/${base%.*}.rst
done
for file in events.doc tlsevents.doc
do
	kernel-doc -rst ${file} > $DST/${file%.*}.rst
done
sphinx-build -b devhelp -E -a . _devhelp
