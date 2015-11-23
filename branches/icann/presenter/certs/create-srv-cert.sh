#!/bin/sh
set -e

X_OPENSSL_CONF=./openssl.conf

mkdir -p server

SRV_REQ=server/server.csr
SRV_KEY=server/server.key
SRV_CRT=server/server.crt

# create a certificate request for a server
#
if test ! -f $SRV_REQ ; then
	echo 'CREATING SERVER REQUEST'
	openssl req -newkey rsa:1024 -keyout $SRV_KEY -out $SRV_REQ
	openssl rsa -in $SRV_KEY -out $SRV_KEY.new
	mv $SRV_KEY.new $SRV_KEY
fi

# issue a certificate based on the server's request
#
if test ! -f $SRV_CRT ; then
	echo 'CREATING SERVER CERT'
	env OPENSSL_CONF=$X_OPENSSL_CONF \
	openssl ca -in $SRV_REQ -out $SRV_CRT
fi

chmod 400 $SRV_KEY
