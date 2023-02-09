# Copyright Robert Bosch GmbH, 2020-2023. Part of the Eclipse Kuksa Project.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#


FROM python:3.10-alpine as build

# alpine-sdk and linux-headers needed for Alpine to build grpcio for aarch64, not needed for platforms with prebuilt grpcio
RUN apk add --no-cache alpine-sdk linux-headers
ADD . /kuksa_gps_feeder
WORKDIR /kuksa_gps_feeder
RUN pip install --upgrade pip
# Note - Installing grpcio (inherited from kuksa-viss-client) takes very long time (40-60 minutes) on Alpine
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
