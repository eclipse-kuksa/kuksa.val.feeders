# vsomeip Documentation

## Generate vsomeip User Guide

vsomeip User Guide is in asciidoc format (requires `asciidoc source-highlight` packages to convert/view.

There is a script that installs dependencies and downloads latest user guide `adoc` format, then converts it to `.html`:
```sh
cd someip2val/doc/
./generate-vsomeip-guide.sh
```

**NOTE**: Also possible to preview the [vsomeipUserGuide.adoc](./documentation/vsomeipUserGuide.adoc) file in vs-code after installing asciidoc extension, but the [vsomeipUserGuide.html](./vsomeipUserGuide.html) works best in a browser.

## Generate vsomeip doxygen

You can (optionally) generate vsomeip doxygen documentation, e.g. in your vscode `build/` directory
```sh
# install recommended tools from vsomeip (may need to reconfigure cmake to detect new tools)
sudo apt-get install asciidoc source-highlight doxygen graphviz

# make sure vs code was started via this script to populate conan environment in build/ dir
cd someip2val/
. ./vscode-conan.sh

# build vsomeip doc target
cd build/
make doc

# output will be generated in:
ls -la _deps/vsomeip3-build/documentation/

# optionally copy to ../doc/
cp -r _deps/vsomeip3-build/documentation/html ../doc

# open generated documentation
cd ..
firefox doc/html/index.html
```

## Containerisation of generic vsomeip examples

Running default vsomeip examples in containers is described in details in [vsomeip-docker/README.md](./vsomeip-docker/README.md)

**NOTE:** Provided Dockerfile and scripts have vsomeip dependencies, they do not apply directly to **someip2val** setup and also don't support communication with external SOME/IP hosts.
