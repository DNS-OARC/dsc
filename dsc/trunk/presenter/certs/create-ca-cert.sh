#!/bin/sh
set -e

X_OPENSSL_CONF=./openssl.conf

mkdir -p private

if test ! -f index.txt ; then touch index.txt ; fi
if test ! -f serial ; then echo 0 > serial ; fi

CA_KEY=private/cakey.pem
CA_CRT=cacert.pem

# create the CA's self-signed root certificate
#
if test ! -f $CA_CRT ; then
	echo 'CREATING CA CERT'
	env OPENSSL_CONF=$X_OPENSSL_CONF \
	openssl req -x509 -days 3000 -newkey rsa -out $CA_CRT
fi
