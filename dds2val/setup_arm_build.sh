#! /bin/bash
if [[ "$TARGETARCH" == "arm64" ]]; then 
  apt-get install -y cmake &&  git clone -b releases/0.10.x https://github.com/eclipse-cyclonedds/cyclonedds.git && cd cyclonedds && mkdir build install && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=../install && cmake --build . --target install && cd .. && export CYCLONEDDS_HOME="$(pwd)/install" && pip3 install cyclonedds --no-binary cyclonedds && export PATH="$(pwd)/install/bin:$PATH"
fi
