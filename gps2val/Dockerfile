# /********************************************************************************
# * Copyright (c) 2020-2023 Contributors to the Eclipse Foundation
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

FROM --platform=$TARGETPLATFORM python:3.10-slim-bookworm as builder

ADD . /kuksa_gps_feeder
WORKDIR /kuksa_gps_feeder
RUN pip install --upgrade pip
RUN pip install --target /kuksa_gps_feeder --no-cache-dir -r requirements.txt


# Debian 12 is bookworm, so the glibc version matches. Distroless is a lot smaller than
# Debian slim versions
# For development add :debug like this
# FROM gcr.io/distroless/base-debian12:debug  to get a busybox shell as well
FROM --platform=$TARGETPLATFORM python:3.10-slim-bookworm

RUN apt update && apt install -y gpsd \
    && rm -rf /var/lib/apt/lists/*

# RUN apk add -yy --no-cache gpsd
COPY --from=builder /kuksa_gps_feeder /kuksa_gps_feeder
COPY --from=builder /bin/sh /bin/sh
WORKDIR /kuksa_gps_feeder

ENV PYTHONUNBUFFERED=yes

ENV GPSD_PORT=29998
ENV GPSD_SOURCE=udp://0.0.0.0:${GPSD_PORT}
ENV GPSD_LISTENER_PORT=2948

EXPOSE ${GPSD_PORT}/udp

# allow overriding GSPD arguments
ENV GPSD_OPTIONS="-S ${GPSD_LISTENER_PORT} ${GPSD_SOURCE}"

# CMD gpsd GPSD_ARGS; ./gpsd_feeder.py
ENTRYPOINT [ "./entrypoint.sh" ]
