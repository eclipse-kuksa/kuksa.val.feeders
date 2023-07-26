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

FROM python:3.10-alpine as build

# alpine-sdk and linux-headers needed for Alpine to build grpcio for aarch64, not needed for platforms with prebuilt grpcio
RUN apk add --no-cache alpine-sdk linux-headers
ADD . /kuksa_gps_feeder
WORKDIR /kuksa_gps_feeder
RUN pip install --upgrade pip
# Note - Installing grpcio (inherited from kuksa-viss-client) takes very long time (40-60 minutes) on Alpine
# Note - if relying on kuksa-client pre-releases you must add --pre to the line below
RUN pip install --target /kuksa_gps_feeder --no-cache-dir -r requirements.txt


FROM python:3.10-alpine

RUN apk add --no-cache gpsd libstdc++
COPY --from=build /kuksa_gps_feeder /kuksa_gps_feeder
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
