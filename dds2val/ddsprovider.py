#! /usr/bin/env python3

########################################################################
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
########################################################################

import logging
import os
import signal
import asyncio
from pathlib import Path
from ddsproviderlib import helper

log = logging.getLogger("ddsprovider")


async def main():
    """Perform the main function activities."""
    logging.basicConfig(level=logging.INFO)
    log.setLevel(logging.INFO)

    console_logger = logging.StreamHandler()
    log.addHandler(console_logger)
    log.info("Starting ddsprovider...")

    if os.environ.get("VEHICLEDATABROKER_DAPR_APP_ID"):
        grpc_metadata = (
            ("dapr-app-id", os.environ.get("VEHICLEDATABROKER_DAPR_APP_ID")),
        )
    else:
        grpc_metadata = None

    if os.environ.get("TOKEN"):
        token = os.environ.get("TOKEN")
    else:
        token = None
        log.info(
            "No token specified. This means no connection to a server/databroker with enabled authorization is possible"
        )

    if os.environ.get("DAPR_GRPC_PORT"):
        log.warning("DAPR_GRPC_PORT is deprecated, please use VDB_PORT instead.")
        port = os.environ.get("DAPR_GRPC_PORT")
    else:
        port = os.environ.get("VDB_PORT", "55555")
    databroker_address = os.environ.get("VDB_ADDRESS", "127.0.0.1") + ":" + port

    mappingfile = os.environ.get(
        "MAPPING_FILE", str(Path(__file__).parent / "mapping/latest/mapping.yml")
    )

    # Collect data for TLS connection, for now default is no TLS
    # To keep backward compatibility not using TLS is default
    if os.environ.get("VDB_ROOT_CA_PATH"):
        root_ca_path = os.environ.get("VDB_ROOT_CA_PATH")
    else:
        root_ca_path = None

    if os.environ.get("VDB_TLS_SERVER_NAME"):
        tls_server_name = os.environ.get("VDB_TLS_SERVER_NAME")
    else:
        tls_server_name = None

    ddsprovider = helper.Ddsprovider(root_ca_path, tls_server_name)

    # Handler for Ctrl-C and Kill signal
    def signal_handler(signal_received, _frame):
        log.info("Received signal %s, stopping", signal_received)
        ddsprovider.stop()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    await ddsprovider.start(
        databroker_address=databroker_address,
        grpc_metadata=grpc_metadata,
        mappingfile=mappingfile,
        token=token,
    )


if __name__ == "__main__":  # pragma: no cover
    LOOP = asyncio.get_event_loop()
    LOOP.add_signal_handler(signal.SIGTERM, LOOP.stop)
    LOOP.run_until_complete(main())
    LOOP.close()
