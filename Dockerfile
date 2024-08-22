## Dockerfile for testing on macos

FROM davydoff/fedora-libfuse:v1-f40

# for the umfs to run
RUN dnf -y install glib2-devel

# for neovim 
RUN dnf -y install git clang-tools-extra bear htop

RUN mkdir /root/.config

# COPY ./nvim /root/.config/nvim

WORKDIR /root/umfs
