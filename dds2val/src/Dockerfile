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
ARG TARGETPLATFORM
ARG BUILDPLATFORM

FROM --platform=$TARGETPLATFORM python:3.9-slim-bullseye as builder

RUN echo "-- Running on $BUILDPLATFORM, building for $TARGETPLATFORM"

RUN apt-get update -qqy && apt-get upgrade -qqy && apt-get install -qqy binutils g++ git

COPY . /

RUN python3 -m venv /opt/venv

ENV PATH="/opt/venv/bin:$PATH"

RUN /opt/venv/bin/python3 -m pip install --upgrade pip \
    && pip3 install --no-cache-dir -r requirements.txt

RUN pip3 install wheel scons && pip3 install pyinstaller patchelf==0.17.0.0 staticx
WORKDIR /idls
RUN ./generate_py_dataclass.sh

WORKDIR /
ENV PATH="/idls:$PATH"
# The cyclonedds module uses a dynamic library which is not automatically discovered (--add-binary)
RUN pyinstaller --clean --paths=. --add-binary="/opt/venv/lib/python3.9/site-packages/cyclonedds.libs/libddsc-62ba48e6.so.0.11.0:cyclonedds.libs/" --add-data="mapping.yml:." -F -s ddsfeeder.py 

WORKDIR /dist
RUN staticx ddsfeeder ddsfeeder-exe

# Runner stage, to copy in the virtual environment and the app
FROM scratch

LABEL org.opencontainers.image.source="https://github.com/eclipse/kuksa.val.feeders"

LABEL org.opencontainers.image.licenses="APACHE-2.0"

# needed as /dist/binary unpacks and runs from /tmp
WORKDIR /tmp
# optional volume mapping
WORKDIR /conf

WORKDIR /dist

COPY --from=builder /dist/ddsfeeder-exe .

ENV PATH="/dist:$PATH"

# useful dumps about feeding values
ENV LOG_LEVEL="info"

ENV PYTHONUNBUFFERED=yes

ENTRYPOINT ["./ddsfeeder-exe"]