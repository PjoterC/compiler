FROM ubuntu:20.04

ARG USERNAME=compilerdev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

SHELL ["/bin/bash", "-c"]

RUN groupadd --gid $USER_GID $USERNAME \
  && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
  && apt-get update \
  && apt-get install -y \
  bear \
  bison \
  build-essential \
  clang-12 \
  curl \
  flex \
  git \
  libedit-dev \
  libgmp-dev \
  libnuma-dev \
  libz-dev \
  lldb-12 \
  llvm-12 \
  sudo \
  && rm -rf /var/lib/apt/lists/* \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
  && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME

RUN curl --proto '=https' --tlsv1.2 -sSf https://get-ghcup.haskell.org | \
  BOOTSTRAP_HASKELL_NONINTERACTIVE=1 \
  BOOTSTRAP_HASKELL_GHC_VERSION=9.0.2 \
  BOOTSTRAP_HASKELL_INSTALL_HLS=1 \
  BOOTSTRAP_HASKELL_HLS_VERSION=2.4.0.0 \
  BOOTSTRAP_HASKELL_VERBOSE=1 \
  sh \
  && source /home/compilerdev/.ghcup/env && echo "source /home/compilerdev/.ghcup/env" >> /home/compilerdev/.bashrc \
  && stack install BNFC alex happy

ENTRYPOINT ["bash"]
