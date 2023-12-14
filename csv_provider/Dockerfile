# /********************************************************************************
# * Copyright (c) 2023 Contributors to the Eclipse Foundation
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


# Build stage, to create a Virtual Environent
FROM --platform=$TARGETPLATFORM python:3.10-slim-bookworm as builder

ARG TARGETPLATFORM
ARG BUILDPLATFORM

RUN echo "-- Running on $BUILDPLATFORM, building for $TARGETPLATFORM"

RUN apt update && apt -yy install binutils git

RUN pip install --upgrade --no-cache-dir pip build pyinstaller

COPY requirements.txt /

RUN pip3 install --no-cache-dir -r requirements.txt

# Copy "all" files first when dependencies have been installed to reuse
# cached layers as much as possible

COPY . /

RUN pyinstaller --clean -F -s provider.py

WORKDIR /dist

WORKDIR /data
COPY ./signals.csv ./signals.csv

# Runner stage, to copy in the virtual environment and the app
# Debian 12 is bookworm, so the glibc version matches. Distroless is a lot smaller than
# Debian slim versions
# For development add :debug like this
# FROM gcr.io/distroless/base-debian12:debug  to get a busybox shell as well
FROM gcr.io/distroless/base-debian12

WORKDIR /dist

COPY --from=builder /dist/* .
COPY --from=builder /data/ ./

# pyinstaller doesn't pick up transient libz dependency, so copying it manually
COPY --from=builder /usr/lib/*-linux-gnu/libz.so.1 /lib/

ENV PATH="/dist:$PATH"

# useful dumps about feeding values
ENV LOG_LEVEL="info"

ENV PYTHONUNBUFFERED=yes

ENTRYPOINT ["./provider"]
