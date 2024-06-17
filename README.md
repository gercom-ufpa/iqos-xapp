# Application for Intelligent QoS Management for O-RAN Networks (IQoS-xApp)

Add a paragraph about the application here...

## Requirements

- python3-dev;
- libsctp-dev;
- librdkafka-dev;
- ninja-build;
- swig v4.1 or newer;

## Getting Started

## Using Docker image

```shell
docker container run -itd --name iqos-xapp -e NEAR_RIC_IP="<NEAR_RIC_IP>" muriloavlis/iqos-xapp:latest
```

### Building the xApp

Clone the xApp repository with the following command.

```shell
git clone --recurse-submodules -j8 git@github.com:gercom-ufpa/iqos-xapp.git
```

Copy service models libraries to system.

```shell
cd iqos-xapp
sudo mkdir -p /usr/local/etc/
sudo cp -R libs/serviceModels /usr/local/lib/flexric
```

Build the xApp.

```shell
mkdir -p build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DE2AP_VERSION=E2AP_V3 -DKPM_VERSION=KPM_V3_00 ..
ninja xapp_iqos
```

### Running the xApp

After building, the xApp can be run with the following command.

```shell
./xapp_iqos -c ../configs/xApp.conf 
```