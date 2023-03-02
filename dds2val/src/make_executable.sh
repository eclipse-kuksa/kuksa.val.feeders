if [[ "$TARGETARCH" == "arm64" ]]; then 
  pyinstaller --clean --paths=. --add-data="mapping.yml:." -F -s ddsfeeder.py; else pyinstaller --clean --paths=. --collect-binaries cyclonedds --add-data="mapping.yml:." -F -s ddsfeeder.py 
fi
