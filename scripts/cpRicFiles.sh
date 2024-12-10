#!/bin/bash

## output dir
mkdir -p out

## binary
mkdir -p out/bin
cp ./build/xapp_iqos out/bin

## service models
mkdir -p out/serviceModels

cp ./build/libs/flexric/src/sm/kpm_sm/kpm_sm_v03.00/libkpm_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/rc_sm/librc_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/mac_sm/libmac_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/slice_sm/libslice_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/pdcp_sm/libpdcp_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/gtp_sm/libgtp_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/tc_sm/libtc_sm.so out/serviceModels
cp ./build/libs/flexric/src/sm/rlc_sm/librlc_sm.so out/serviceModels