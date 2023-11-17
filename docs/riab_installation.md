# SD-RAN in a Box Installation

## Requirements

- Intel CPU and Haswell microarchitecture or beyond; at least 4 cores
- Ubuntu 18.04 (under test with Ubuntu 20.04)
- At least 16GB
- At least 50GB (recommendation: 100GB)

## Installation

Clone the RIAB repository

```bash
git clone https://github.com/onosproject/sdran-in-a-box
```

Set the branch

```bash
git checkout master
```

Deploy RIAB on the machine or VM

```bash
make riab OPT=oai VER=stable
```