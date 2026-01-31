# KTPAMXX Build Environment
# Ubuntu 22.04 (glibc 2.35) for compatibility with older servers
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        g++-multilib \
        gcc-multilib \
        python3 \
        python3-pip \
        python3-venv \
        git \
        nasm \
        lib32z1-dev \
        libc6-dev-i386 \
        linux-libc-dev:i386 \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /build

# Copy build script
COPY docker-build.sh /build/docker-build.sh
RUN chmod +x /build/docker-build.sh

CMD ["/build/docker-build.sh"]
