#---------------------------------------------------------------------
# IQoS-xApp IMAGE
#---------------------------------------------------------------------

## ubuntu:24.04
ARG BASE_IMAGE=ubuntu:noble

FROM $BASE_IMAGE

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Belem
ENV NEAR_RIC_IP=${NEAR_RIC_IP}

#install pkg/repo needed to run
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get upgrade --yes && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
       psmisc \
       gpg \
       wget \
       libsctp1 \
       python3 \
       python3-pip \
       net-tools && \
    apt-get autoremove -y && \
    apt-get autoclean -y

WORKDIR /

### Copy service models and binary from git action build
COPY out/serviceModels /usr/local/lib/flexric
COPY out/bin/xapp_iqos /usr/local/bin/xapp_iqos

## Copy Config file
COPY configs/xApp.conf /usr/local/etc/flexric/flexric.conf

## Copy initialization script
COPY scripts/run.sh /run.sh
RUN chmod +x /run.sh

RUN ldconfig && \
    echo "--- Check shared objects dependencies for executable nearRT-RIC ---" && \
    ldd /usr/local/bin/xapp_iqos && \
    echo "--- Check shared objects dependencies for flexric shared libraries ---" && \
    ldd /usr/local/lib/flexric/*.so

LABEL br.ufpa.gercom.image.title="IQoS-xApp" \
      br.ufpa.gercom.image.description="xApp for QoS management in O-RAN network contexts" \
      br.ufpa.gercom.image.url="https://github.com/gercom-ufpa/iqos-xapp" \
      br.ufpa.gercom.image.source="https://github.com/gercom-ufpa/iqos-xapp" \
      br.ufpa.gercom.image.maintainer="GT-IQoS/UFPA" \
      br.ufpa.gercom.image.contact="murilosilva@itec.ufpa.br" \
      br.ufpa.gercom.image.version="0.0.1" \
      br.ufpa.gercom.image.licenses=""

## E42 -- xApp
EXPOSE 36422/sctp

ENTRYPOINT ["/run.sh"]