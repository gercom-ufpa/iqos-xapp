# Application for Intelligent QoS Management for O-RAN Networks (IQoS-xApp)

Add a paragraph about the application here...

## Requirements

- python3-dev;
- libsctp-dev;

## Getting Started

### Installing the xApp

Clone the xApp repository with the following command.

```shell
git clone --recurse-submodules -j8 git@github.com:gercom-ufpa/iqos-xapp.git
```

Copy service models libraries to system.

```shell
sudo mkdir -p /usr/local/etc/
sudo cp -R libs/serviceModels /usr/local/etc/flexric
```

Build the xApp.

```shell
cd iqos-xapp
mkdir -p build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DE2AP_VERSION=E2AP_V3 -DKPM_VERSION=KPM_V3_00 ..
ninja xapp_iqos
```

### Running the xApp

After building, the xApp can be run with the following command.

```shell
./xapp_iqos -c ../configs/xApp.conf 
```
