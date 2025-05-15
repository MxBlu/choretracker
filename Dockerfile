FROM ubuntu AS build

# Necessary utility packages
RUN apt-get update && apt-get install -y \ 
      git zip pkg-config ninja-build wget curl \ 
      lsb-release software-properties-common gnupg \
      ca-certificates

# Install clang 19
RUN wget https://apt.llvm.org/llvm.sh && \ 
      chmod +x llvm.sh && \ 
      ./llvm.sh 19
ENV PATH="$PATH:/usr/lib/llvm-19/bin"
RUN apt-get update && apt-get install -y clang-tools-19

# Install CMake
RUN bash -c "$(wget -O - https://apt.kitware.com/kitware-archive.sh)" && \
      apt-get update && apt-get install -y cmake

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg /vcpkg && /vcpkg/bootstrap-vcpkg.sh
ENV VCPKG_DEFAULT_BINARY_CACHE=/vcpkg-cache

WORKDIR /src

COPY src src
COPY include include 
COPY triplets triplets
COPY CMakeLists.txt .
COPY vcpkg.json vcpkg-configuration.json .
RUN --mount=type=cache,target=/vcpkg-cache cmake \
      -DVCPKG_TARGET_TRIPLET="x64-linux-release" \
      -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -B build  .
RUN --mount=type=cache,target=/vcpkg-cache cmake --build build --config Release --target choretracker -v
RUN strip build/choretracker

FROM ubuntu AS runtime

RUN DEBIAN_FRONTEND=noninteractive apt update && apt install -y tzdata && \
      apt clean && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/vcpkg_installed/x64-linux-release/lib/* /usr/lib/
COPY --from=build /src/build/choretracker /app/choretracker

CMD [ "/app/choretracker" ]