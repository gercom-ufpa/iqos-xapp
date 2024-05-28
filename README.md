# Application for Intelligent QoS Management for O-RAN Networks (IQoS-xApp)

Add a paragraph about the application here...

## Requirements

- Near-RT RIC FlexRIC;

## Getting Started

### Installing the xApp

After installing FlexRIC, clone the xApp repository with the following command.

```shell
git clone --recurse-submodules -j8 git@github.com:gercom-ufpa/iqos-xapp.git
```

Build the xApp.

```shell
cd iqos-xapp
mkdir -p build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DE2AP_VERSION=E2AP_V3 -DKPM_VERSION=KPM_V3_00 ..
ninja iqos-xapp
```

### Running the xApp

After building, the xApp can be run with the following command.

```shell
./iqos-xapp -c xApp.conf
```
