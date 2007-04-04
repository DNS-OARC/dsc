#!/bin/sh
set -e

X_OPENSSL_CONF=./openssl.conf

mkdir -p client

CLT_REQ=client/client.csr
CLT_KEY=client/client.key
CLT_CRT=client/client.crt

# create a certificate request for a client
#
if test ! -f $CLT_REQ ; then
	echo 'CREATING CLIENT REQUEST'
	openssl req -newkey rsa:1024 -keyout $CLT_KEY -out $CLT_REQ
fi

# issue a certificate based on the client's request
#
if test ! -f $CLT_CRT ; then
	echo 'CREATING CLIENT CERT'
	env OPENSSL_CONF=$X_OPENSSL_CONF \
	openssl ca -in $CLT_REQ -out $CLT_CRT
fi

openssl rsa -in $CLT_KEY -out $CLT_KEY.new
mv $CLT_KEY.new $CLT_KEY
chmod 400 $CLT_KEY

CN=`openssl x509 -in $CLT_CRT -noout -subject -nameopt multiline | awk '/commonName/ {print $3}'`
OU=`openssl x509 -in $CLT_CRT -noout -subject -nameopt multiline | awk '/organizationalUnitName/ {print $3}'`
mkdir -p client/$OU/$CN
mv $CLT_KEY client/$OU/$CN/client.key
mv $CLT_REQ client/$OU/$CN/client.csr
mv $CLT_CRT client/$OU/$CN/client.crt

openssl rsa -in client/$OU/$CN/client.key   > client/$OU/$CN/client.pem
openssl x509 -in client/$OU/$CN/client.crt >> client/$OU/$CN/client.pem

exit 0
