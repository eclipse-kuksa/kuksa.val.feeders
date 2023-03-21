import logging
import os
import signal
import asyncio
from pathlib import Path
from ddsproviderlib import helper

log = logging.getLogger("ddsprovider")

async def main():
    """Perform the main function activities."""
    logging.basicConfig(level=logging.DEBUG)
    log.setLevel(logging.DEBUG)

    console_logger = logging.StreamHandler()
    log.addHandler(console_logger)
    log.info("Starting ddsprovider...")

    if os.environ.get("VEHICLEDATABROKER_DAPR_APP_ID"):
        grpc_metadata = (
            ("dapr-app-id", os.environ.get("VEHICLEDATABROKER_DAPR_APP_ID")),
        )
    else:
        grpc_metadata = None

    databroker_address = "127.0.0.1:" + os.environ.get("DAPR_GRPC_PORT", "55555")

    mappingfile = os.environ.get(
        "MAPPING_FILE", str(Path(__file__).parent / "mapping.yml")
    )

    ddsprovider = helper.Ddsprovider()

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
    )


if __name__ == "__main__":  # pragma: no cover
    LOOP = asyncio.get_event_loop()
    LOOP.add_signal_handler(signal.SIGTERM, LOOP.stop)
    LOOP.run_until_complete(main())
    LOOP.close()