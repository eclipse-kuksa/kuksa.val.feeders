#!/bin/bash

#********************************************************************************
# Copyright (c) 2023 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License 2.0 which is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#*******************************************************************************/

if [ -z "$(which asciidoc)" ] || [ -z "$(which source-highlight)" ]; then
	echo "# Installing asciidoc..."
	sudo apt-get install -y --no-install-recommends asciidoc source-highlight
fi

echo "# Downloading vsomeipUserGuide.adoc ..."
wget -q https://raw.githubusercontent.com/COVESA/vsomeip/master/documentation/vsomeipUserGuide -O vsomeipUserGuide.adoc

echo "# Generating vsomeipUserGuide.html ..."
asciidoc -v -b html -o vsomeipUserGuide.html vsomeipUserGuide.adoc