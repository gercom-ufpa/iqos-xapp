<!-- Credits Open Networking Foundation (ONF) -->

## Self-signed Certificates (Test Environment Only!!)
This folder contains self-signed certificates for use in testing. _DO NOT USE THESE
CERTIFICATES IN PRODUCTION!_

The certificates were generated based on the script 
[generate_certs.sh](/deployments/helm-chart/qmai/files/certs/generate_certs.sh), as shown below:

```bash
generate-certs.sh qmai.gercom.ufpa.br
```

In this folder they **must** be (re)named
* tls.cacrt
* tls.crt
* tls.key

Use
```bash
openssl x509 -in tls.crt -text -noout
```
to verify the contents (especially the subject).