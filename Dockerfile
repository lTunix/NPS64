FROM ubuntu:22.04@sha256:b8b6ee6aa931ecd9d0d952abc34dc0e5f7c6a30c6bb71b079fe399fde0329c02
WORKDIR /work
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends clang lld llvm nasm make git curl ca-certificates python3 python3-pil fonts-dejavu-core unzip libssl3 libicu70 && rm -rf /var/lib/apt/lists/*
# OpenOrbis v0.5.4 ships a .NET Core 3 packager requiring libssl 1.1.
RUN curl -fL --retry 3 https://archive.ubuntu.com/ubuntu/pool/main/o/openssl/libssl1.1_1.1.1f-1ubuntu2.24_amd64.deb -o /work/libssl1.1.deb && echo '7cf39d70a639017d1dd7c8d36daa2258063608688e449fddf40ffdd46f992a78  /work/libssl1.1.deb' | sha256sum -c - && dpkg -i /work/libssl1.1.deb && rm /work/libssl1.1.deb
ENV OO_PS4_TOOLCHAIN=/work/toolchain/OpenOrbis/PS4Toolchain
ENV DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
CMD ["bash", "scripts/build.sh"]
