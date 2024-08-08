## Dockerfile for testing on macos

FROM davydoff/fedora-libfuse:v1-f40

RUN dnf -y install git glib2-devel

COPY . /root/umfs

WORKDIR /root/umfs