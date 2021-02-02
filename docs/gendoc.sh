#!/bin/sh

SRC=..
DST=_kdoc

SOURCES="libesmtp.h message-callbacks.c
smtp-api.c  smtp-auth.c  smtp-etrn.c  smtp-tls.c errors.c
auth-client.c headers.c
"

mkdir -p $DST
for file in $SOURCES
do
	base=${file##*/}
	kernel-doc -rst $SRC/${file} > $DST/${base%.*}.rst
done
sphinx-build . _build
