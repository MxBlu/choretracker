FROM gcc:15 AS build

RUN apt-get update && apt-get install -y git zip
RUN cd /tmp && wget https://github.com/Kitware/CMake/releases/download/v4.0.2/cmake-4.0.2-linux-x86_64.tar.gz \
   && tar -xvf cmake-4.0.2-linux-x86_64.tar.gz \
   && rm -rf cmake-4.0.2-linux-x86_64/man \
   && cp -r cmake-4.0.2-linux-x86_64/* /usr/local
RUN git clone https://github.com/microsoft/vcpkg /vcpkg && /vcpkg/bootstrap-vcpkg.sh

WORKDIR /src
COPY vcpkg.json vcpkg-configuration.json .
RUN /vcpkg/vcpkg install --x-install-root=./build --triplet="x64-linux-release"

COPY src src
COPY include include 
COPY CMakeLists.txt .
RUN cmake -DVCPKG_TARGET_TRIPLET="x64-linux-release" -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_CXX_FLAGS="-static-libstdc++" -B build .
RUN cmake --build build --config Release --target choretracker
RUN strip build/choretracker

FROM ubuntu as runtime

COPY --from=build /src/build/vcpkg_installed/x64-linux-release/lib/* /usr/lib/
COPY --from=build /src/build/choretracker /app/choretracker

CMD [ "/app/choretracker" ]