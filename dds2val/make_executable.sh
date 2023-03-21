if [[ "$TARGETARCH" == "arm64" ]]; then 
  pyinstaller --clean --paths=. --add-data="mapping.yml:." -F -s ddsprovider.py; else pyinstaller --clean --paths=.  --add-binary="/opt/venv/lib/python3.9/site-packages/cyclonedds.libs:cyclonedds.libs" --add-data="mapping.yml:." -F -s ddsprovider.py 
fi
