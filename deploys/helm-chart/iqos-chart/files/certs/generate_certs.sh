# Credits to Open Networking Foundation (ONF)
# Original script in: https://github.com/onosproject/gnxi-simulators/blob/master/pkg/certs/generate_certs.sh

#!/bin/sh

SUBJBASE="/C=BR/ST=PA/L=Belem/O=UFPA/OU=GERCOM/"
DEVICE=${1:-device1.opennetworking.org}
SUBJ=${SUBJBASE}"CN="${DEVICE}

print_usage() {
    echo "Generate a certificate."
    echo
    echo "Usage:     <devicename>"
    echo "           [-h | --help]"
    echo "Options:"
    echo "    DEVICENAME      e.g. device1.opennetworking.org or localhost"
    echo "    [-h | --help]   Print this help"
    echo "";
}

# Print usage
if [ "${1}" = "-h" -o "${1}" = "--help" ]; then
    print_usage
    exit 0
fi

if [ "${PWD##*/}" != "certs" ]; then
    cd certs
fi

rm -f ${DEVICE}.*

## BEFORE
# Generate private key for CA
openssl genrsa -out ca.key 4096

# Generate a CA ceriticate
openssl req -x509 -new -nodes -key ca.key -sha256 -days 1825 -out tls.cacert -subj $SUBJ

# Generate Server Private Key
openssl req \
        -newkey rsa:4096 \
        -nodes \
        -keyout tls.key \
	      -noout \
        -subj $SUBJ \
	 > /dev/null 2>&1

# Generate Req
openssl req \
        -key tls.key \
        -new -out ${DEVICE}.csr \
        -subj $SUBJ \
	 > /dev/null 2>&1

# Generate x509 with signed CA
openssl x509 \
        -req \
        -in ${DEVICE}.csr \
        -CA tls.cacert \
        -CAkey ca.key \
        -CAcreateserial \
        -days 3650 \
        -sha256 \
        -out tls.crt \
	 > /dev/null 2>&1

rm ${DEVICE}.csr ca.key tls.srl

echo " == Certificate Generated: "${DEVICE}.crt" =="
openssl verify -verbose -purpose sslserver -CAfile onfca.crt ${DEVICE}.crt > /dev/null 2>&1
exit $?
#To see full details run 'openssl x509 -in "${TYPE}${INDEX}".crt -text -noout'