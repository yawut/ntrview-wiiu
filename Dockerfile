# that's right folks we building decaf
FROM debian:buster-slim AS decaf-build
RUN apt-get -y update && apt-get -y install --no-install-recommends \
    zlib1g-dev libcurl4-openssl-dev libssl-dev libuv1-dev libc-ares-dev python3 \
    cmake make gcc g++ git ca-certificates \
&& rm -rf /var/lib/apt/lists

# this is incredible I love it
# https://stackoverflow.com/a/43136160
# https://stackoverflow.com/a/47812096
WORKDIR /decaf-emu
RUN git init && \
    git remote add origin https://github.com/decaf-emu/decaf-emu && \
    git fetch --depth 1 origin e4473ca27848843abfa0a8a68a07bdb0f6998898 && \
    git checkout FETCH_HEAD && \
    git submodule update --init --depth 1

WORKDIR /decaf-emu/build
RUN cmake .. -DDECAF_FFMPEG=OFF -DDECAF_SDL=OFF -DDECAF_VULKAN=OFF -DDECAF_QT=OFF -DDECAF_BUILD_TOOLS=ON && \
    make latte-assembler -j$(nproc)

# build ntrview
FROM devkitpro/devkitppc:20220103
RUN apt-get -y update && apt-get -y install --no-install-recommends \
    xxd \
&& rm -rf /var/lib/apt/lists

COPY --from=decaf-build /decaf-emu/build/obj/latte-assembler /usr/local/bin

WORKDIR /app
CMD mkdir -p build && cd build && cmake .. && make -j$(nproc)
