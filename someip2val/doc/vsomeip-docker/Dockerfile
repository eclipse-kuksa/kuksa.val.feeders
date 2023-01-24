# /********************************************************************************
# * Copyright (c) 2022 Contributors to the Eclipse Foundation
# *
# * See the NOTICE file(s) distributed with this work for additional
# * information regarding copyright ownership.
# *
# * This program and the accompanying materials are made available under the
# * terms of the Apache License 2.0 which is available at
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * SPDX-License-Identifier: Apache-2.0
# ********************************************************************************/

FROM --platform=$BUILDPLATFORM ubuntu:20.04 as builder

ARG TARGETPLATFORM

WORKDIR /workspace

ENV DEBIAN_FRONTEND="noninteractive"
RUN DEBIAN_FRONTEND=noninteractive apt-get update -qqy && \
    apt-get --no-install-recommends install -qqy \
        build-essential pkg-config \
        git gcc g++ cmake \
        libboost-filesystem-dev libboost-thread-dev libboost-log-dev

# checkout sources and dependencies
ENV GIT_SSL_NO_VERIFY=1
RUN git clone https://github.com/COVESA/vsomeip.git

# copy changes to vsomeip from current worktree
COPY vsomeip/. vsomeip/
# make sure build dir is empty
RUN if [ -d /workspace/vsomeip/build/ ]; then rm -rf /workspace/vsomeip/build/; fi

# RUN git clone --recurse-submodules -b v1.46.3 --depth 1 --shallow-submodules -c advice.detachedHead=false https://github.com/grpc/grpc

RUN find vsomeip/

RUN mkdir vsomeip/build && cd vsomeip/build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SIGNAL_HANDLING=1 \
    -DCMAKE_INSTALL_PREFIX:PATH="/workspace/install" ..
	# -DBASE_PATH=/tmp/vsomeip

RUN cd vsomeip/build && make -j $(nproc) all install/strip
RUN cd vsomeip/build && make -j $(nproc) examples
## optional examples from vsomeip
# RUN cd vsomeip/build && make -j $(nproc) hello_world_service hello_world_client
RUN cd vsomeip/build && make -j $(nproc) tools vsomeip_ctrl routingmanagerd

# RUN cd vsomeip/build/wiper_poc && make -j $(nproc) install

FROM --platform=$TARGETPLATFORM ubuntu:20.04 as runtime

LABEL org.opencontainers.image.description SOME/IP Kuksa.VAL feeder

RUN apt-get -qqy update && apt-get install --no-install-recommends -qqy libboost-filesystem-dev libboost-thread-dev libboost-log-dev
# jq, ping might be required to update local config with proper ip addresses
RUN apt-get install -qqy --no-install-recommends jq net-tools iputils-ping

## development helpers, to be removed in final image
# RUN apt-get install -qqy --no-install-recommends iproute2 iperf nano

# reduce image size
RUN apt clean && rm -rf /var/lib/apt/lists/*

# create base dir for vsomeip
WORKDIR /tmp/vsomeip
# enable volume mountpoint
WORKDIR /app/config
COPY --from=builder workspace/vsomeip/config/ /app/config/

WORKDIR /app/lib
COPY --from=builder /workspace/install/lib/. /app/lib/

WORKDIR /app/bin

# RUN printf "alias lss='ls -la'\nalias ..='cd ..'\n" >> /root/.bashrc

COPY --from=builder workspace/vsomeip/build/examples/*-sample /app/bin/
COPY --from=builder workspace/vsomeip/build/tools/vsomeip_ctrl /app/bin/
# COPY --from=builder workspace/vsomeip/build/wiper_poc/wiper-*-poc /app/bin/
# COPY --from=builder workspace/vsomeip/wiper_poc/*.sh /app/bin/
# COPY --from=builder workspace/vsomeip/build/wiper_poc/*.sh /app/bin/
# COPY --from=builder workspace/vsomeip/examples/setup*.sh /app/bin/
COPY --from=builder workspace/vsomeip/examples/run-*.sh /app/bin/


# make sure vsomeip libs are in path
ENV LD_LIBRARY_PATH=/app/lib

## configure as client by default
CMD [ "/app/bin/run-sample.sh" ]
