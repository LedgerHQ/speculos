# Dockerfile to have a container with everything ready to build speculos,
# assuming that neither OpenSSL nor cmocka were updated.
#
# Support Debian buster & Ubuntu Bionic

FROM python:3.8-slim
ENV LANG C.UTF-8

RUN apt-get update
RUN apt-get install -qy \
  cmake \
  gcc-arm-linux-gnueabihf \
  git \
  libc6-dev-armhf-cross \
  libvncserver-dev \
  python3-pip \
  qemu-user-static \
  wget

# There are issues with PYTHONHOME if using distro packages, use pip instead.
RUN pip3 install construct jsonschema mnemonic pycrypto pyelftools pbkdf2 pytest

# Create SHA256SUMS
RUN \
  echo 186c6bfe6ecfba7a5b48c47f8a1673d0f3b0e5ba2e25602dd23b629975da3f35 openssl-1.1.1f.tar.gz >> SHA256SUMS && \
  echo f0ccd8242d55e2fd74b16ba518359151f6f8383ff8aef4976e48393f77bba8b6 cmocka-1.1.5.tar.xz >> SHA256SUMS

# Download dependencies
RUN \
  wget https://www.openssl.org/source/openssl-1.1.1f.tar.gz && \
  wget https://cmocka.org/files/1.1/cmocka-1.1.5.tar.xz

# Verify dependencies integrity
RUN sha256sum --check SHA256SUMS

# Build dependencies
RUN mkdir install

RUN tar xf openssl-1.1.1f.tar.gz
RUN cd openssl-1.1.1f && ./Configure --cross-compile-prefix=arm-linux-gnueabihf- no-asm no-threads no-shared no-sock linux-armv4 --prefix=/install && make -j CFLAGS=-mthumb && make install_sw

RUN mkdir cmocka
RUN tar xf cmocka-1.1.5.tar.xz
RUN cd cmocka && cmake ../cmocka-1.1.5 -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc -DCMAKE_C_FLAGS=-mthumb -DWITH_STATIC_LIB=true -DCMAKE_INSTALL_PREFIX=/install && make install

# Remove unnecessary files
RUN rm -rf \
  cmocka/ \
  cmocka-1.1.5/ \
  cmocka-1.1.5.tar.xz \
  openssl-1.1.1f/ \
  openssl-1.1.1f.tar.gz \
  SHA256SUMS

RUN apt-get clean && rm -rf /var/lib/apt/lists/

CMD ["/bin/bash"]
