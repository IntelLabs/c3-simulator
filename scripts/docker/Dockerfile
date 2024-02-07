FROM mcr.microsoft.com/devcontainers/base:ubuntu-20.04
ARG DEBIAN_FRONTEND=noninteractive

ARG C3_USER_UID
ARG C3_USER_GID

ARG C3_WORKDIR=/c3_workdir
ARG C3_USER=c3_user
ARG C3_USER_HOME=/home/$C3_USER

ARG NASM_GIT=https://github.com/netwide-assembler/nasm.git
ARG NASM_SRC=/opt/nasm/src
ARG NASM_DST=/opt/nasm/nasm-2.15.05
ARG NASM_REL=nasm-2.15.05

ARG SIMICS_INSTALL_DIR=/opt/simics/simics-6
ARG SIMICS_ISPM="intel-simics-package-manager-1.7.5"
ARG SIMICS_ISPM_PKG="${SIMICS_ISPM}-linux64.tar.gz"
ARG SIMICS_BUNDLE_PKG="simics-6-packages-2023-31-linux64.ispm"

ARG MBUILD_GIT_URL="https://github.com/intelxed/mbuild.git"
ARG MBUILD_GIT_TAG="v2022.07.28"
ARG XED_GIT_URL="https://github.com/intelxed/xed.git"
ARG XED_GIT_TAG="v2022.10.11"
ARG XED_INSTALL_PATH="${C3_WORKDIR}/lib/xed/kits/xed"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update && apt-get upgrade -y \
    && apt-get install --no-install-recommends -y \
    acpica-tools \
    autoconf \
    automake \
    bison \
    build-essential \
    clang-format \
    clang-tidy \
    wget \
    doxygen \
    dwarves \
    flex \
    gawk \
    gcc-9 \
    g++-9 \
    git \
    graphviz \
    libatk1.0-dev \
    libatk-bridge2.0-dev \
    libelf-dev \
    libgtk-3-dev \
    libssl-dev \
    libtinfo-dev \
    llvm \
    ninja-build \
    python3 \
    python3-pip \
    zstd \
    bc \
    cpio \
    file \
    libjson-perl \
    dbus-x11 \
    openssh-server \
    xterm \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*




RUN pip3 install --no-cache-dir\
    cpplint \
    pre-commit \
    pytest-xdist

# Add c3_user
RUN groupadd -g ${C3_USER_GID} ${C3_USER} && \
    useradd -l ${C3_USER} -u ${C3_USER_UID} -g ${C3_USER_GID} && \
    mkhomedir_helper ${C3_USER} && \
    mkdir -p ${C3_USER_HOME} && \
    chown -R ${C3_USER}:${C3_USER} ${C3_USER_HOME}

# Install Simics
COPY ${SIMICS_BUNDLE_PKG} ${C3_USER_HOME}
COPY ${SIMICS_ISPM_PKG} ${C3_USER_HOME}
RUN mkdir -p /opt/simics/ispm && pushd /opt/simics/ispm && \
    tar xzf ${C3_USER_HOME}/$SIMICS_ISPM_PKG && \
    /opt/simics/ispm/$SIMICS_ISPM/ispm -y packages \
            --install-bundle ${C3_USER_HOME}/$SIMICS_BUNDLE_PKG \
            --install-dir $SIMICS_INSTALL_DIR && \
    ln -s $SIMICS_INSTALL_DIR/$(ls $SIMICS_INSTALL_DIR | \
                                grep simics-6 | sort -V | tail -n1) \
            $SIMICS_INSTALL_DIR/simics-latest && \
    $SIMICS_INSTALL_DIR/simics-latest/bin/addon-manager -b \
            -s $SIMICS_INSTALL_DIR/$(ls $SIMICS_INSTALL_DIR \
                                     | grep simics-gdb \
                                     | sort -V | tail -n1) \
            -s $SIMICS_INSTALL_DIR/$(ls $SIMICS_INSTALL_DIR \
                                     | grep simics-qsp-clear-linux \
                                     | sort -V | tail -n1) \
            -s $SIMICS_INSTALL_DIR/$(ls $SIMICS_INSTALL_DIR \
                                     | grep simics-qsp-cpu \
                                     | sort -V | tail -n1) \
            -s $SIMICS_INSTALL_DIR/$(ls $SIMICS_INSTALL_DIR \
                                     | grep simics-qsp-x86 \
                                     | sort -V | tail -n1) && \
    popd && \
    rm -rf /opt/simics/ispm && \
    chown -R ${C3_USER}:${C3_USER} ${SIMICS_INSTALL_DIR}

# Install nasm
RUN git clone --single-branch --depth 1 -b ${NASM_REL} ${NASM_GIT} ${NASM_SRC} && \
    pushd ${NASM_SRC} && \
    echo "NOTE: dasm and Ndisasm docs are named wrong in the repo" && \
    mv nasm.txt nasm.1 && mv ndisasm.txt ndisasm.1 && \
    sh autogen.sh && \
    sh configure --prefix="${NASM_DST}" --exec-prefix="${NASM_DST}" && \
    make -j$nproc install && \
    popd && \
    rm -rf ${NASM_SRC}
ENV PATH="/opt/nasm/nasm-2.15.05/bin:${PATH}"

# Install CMake
RUN mkdir -p /opt/cmake && pushd /opt/cmake && \
    wget -q https://github.com/Kitware/CMake/releases/download/v3.28.0-rc5/cmake-3.28.0-rc5-linux-x86_64.sh && \
    sh cmake-3.28.0-rc5-linux-x86_64.sh --skip-license  && \
    rm cmake-3.28.0-rc5-linux-x86_64.sh
ENV PATH="/opt/cmake/bin:${PATH}"

# Add folders
RUN mkdir -p $C3_WORKDIR && \
    mkdir -p $C3_WORKDIR/glibc && \
    mkdir -p $C3_WORKDIR/llvm && \
    mkdir -p $C3_WORKDIR/linux && \
    mkdir -p $C3_WORKDIR/edk2_src && \
    chown -R ${C3_USER}:${C3_USER} ${C3_WORKDIR}
WORKDIR $C3_WORKDIR

# Configure zsh shell
RUN cp /etc/zsh/newuser.zshrc.recommended ${C3_USER_HOME}/.zshrc && \
    echo "cd /c3_workdir" >> ${C3_USER_HOME}/.zshrc && \
    usermod --shell /usr/bin/zsh ${C3_USER}

# Set up SSH
RUN mkdir -p /var/run/sshd && \
    echo ${C3_USER}:${C3_USER} | chpasswd && \
    echo 'root:root' | chpasswd && \
    sed -i 's/^#PasswordAuthentication yes$/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/^#X11UseLocalhost yes$/X11UseLocalhost no/' /etc/ssh/sshd_config && \
    mkdir -p ${C3_USER_HOME}/.ssh
COPY c3_docker.key.pub ${C3_USER_HOME}/.ssh/authorized_keys
RUN chmod o-rx ${C3_USER_HOME}/.ssh/authorized_keys && \
    chown ${C3_USER}:${C3_USER} ${C3_USER_HOME}/.ssh/authorized_keys
EXPOSE 22

# Bootstrap Simics project
RUN su c3_user -c "$SIMICS_INSTALL_DIR/simics-latest/bin/project-setup --ignore-existing-files ."

# Install xed
RUN mkdir -p lib/mbuild && \
    git -C lib/mbuild clone -b ${MBUILD_GIT_TAG} --depth 1 ${MBUILD_GIT_URL} . && \
    mkdir -p lib/xed && \
    git -C lib/xed clone -b ${XED_GIT_TAG} --depth 1 ${XED_GIT_URL} . && \
    pushd lib/xed && \
    ./mfile.py install --extra-flags=-fPIC --install-dir=${XED_INSTALL_PATH} && \
    chown -R ${C3_USER}:${C3_USER} $C3_WORKDIR/lib/xed && \
    chown -R ${C3_USER}:${C3_USER} $C3_WORKDIR/lib/mbuild

# Download patchelf
RUN su c3_user -c "wget -q https://codeload.github.com/NixOS/patchelf/tar.gz/refs/tags/0.10"

CMD ["/usr/sbin/sshd", "-D"]
