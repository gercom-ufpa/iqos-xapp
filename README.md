# Application for Intelligent QoS Management for O-RAN Networks (IQoS-xApp)

Add a paragraph about the application here...

## Requirements

- python3-dev;
- libsctp-dev;
- librdkafka-dev;
- ninja-build;
- swig v4.1 or newer;

## Getting Started

## Flexric

To use the xApp, it is necessary to utilize the near RT RIC. In this work's context, we use Flexric. Therefore, before compiling and running the xApp, we first need to deploy Flexric.

### Requirements

- Cmake (at least v3.22);
- SWIG ((at least v4.1);
- GCC (gcc-10, gcc-12, or gcc-13) - no support for gcc-11;

### Installing SWIG

```shell
git clone https://github.com/swig/swig.git
cd swig
git checkout release-4.1
./autogen.sh
./configure --prefix=/usr/
make -j8
make install
```
### Installing Dependencies 

The required dependencies can be installed using the following command:

```shell
sudo apt install libsctp-dev python3.8 cmake-curses-gui libpcre2-dev python3-dev
```
### Clone the FlexRIC Project, Build, and Install It
After installing all prerequisites, clone the Flexric repository using the following command:

```shell
git clone https://gitlab.eurecom.fr/mosaic5g/flexric.git 
# Recommended to use the dev branch
git checkout <here put the release tag>
```
Then, build the near RT RIC using the following commands:

```shell
mkdir -p build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DE2AP_VERSION=E2AP_V3 -DKPM_VERSION=KPM_V3_00 ..
ninja install 
```

NOTE: We use version 3 of KPM and E2AP, which are compatible with our xApp.

### Flexric Deployment

```shell
cd ~/flexric/build/examples/ric/
./nearRT-RIC
```

In addition to Flexric, we will need the E2 Node agents. The gNB can be deployed as a monolithic instance or using the CU/DU split:

- gNB-mono
```shell
cd ~/flexric/build/examples/emulator/agent
./emu_agent_gnb
```

- gNB-split

```shell
cd ~/flexric/build/examples/emulator/agent
./emu_agent_gnb_cu
./emu_agent_gnb_du
```

Once the deployment is successful, the xApp can be used.

NOTE: This tutorial is based on the official [Flexric](https://gitlab.eurecom.fr/mosaic5g/flexric/-/tree/dev?ref_type=heads#flexric) documentation.

## Using Docker image

```shell
docker container run -itd --name iqos-xapp -e NEAR_RIC_IP="<NEAR_RIC_IP>" muriloavlis/iqos-xapp:latest
```

### Building the xApp

Clone the xApp repository with the following command.

```shell
git clone --recurse-submodules -j8 git@github.com:gercom-ufpa/iqos-xapp.git
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

NOTE: In the xApp.conf file we can set the Flexric IP. 