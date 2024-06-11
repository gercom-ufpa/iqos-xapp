#!/bin/bash

## Setup RIC IP
sed -i "s/NEAR_RIC_IP = 127.0.0.1/NEAR_RIC_IP = ${NEAR_RIC_IP}/g" /usr/local/etc/flexric/flexric.conf

## Exec xApp bin
stdbuf -o0 xapp_iqos
