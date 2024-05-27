# Copyright (C) 2024 Intel Corporation
# SPDX-License-Identifier: MIT

ARG C3_DOCKER_BASE_TAG=c3_base
FROM ${C3_DOCKER_BASE_TAG}
ARG DEBIAN_FRONTEND=noninteractive

ARG C3_USER_UID
ARG C3_USER_GID

ARG C3_WORKDIR=/c3_workdir
ARG C3_USER=c3_user
ARG C3_USER_HOME=/home/$C3_USER

ARG SIMICS_INSTALL_DIR=/opt/simics/simics-6

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Add c3_user, if needed
RUN if (! getent group ${C3_USER_GID}) ; then \
      groupadd -g ${C3_USER_GID} ${C3_USER} ; \
    fi
RUN if (! getent passwd ${C3_USER_UID}) ; then \
      useradd -M -l ${C3_USER} -u ${C3_USER_UID} -g ${C3_USER_GID} ; \
    else \
      usermod -l ${C3_USER} $(id -n -u ${C3_USER_UID}) ; \
    fi
RUN chown -R ${C3_USER_UID}:${C3_USER_GID} ${C3_USER_HOME} && \
    chown -R ${C3_USER_UID}:${C3_USER_GID} ${SIMICS_INSTALL_DIR} && \
    chown -R ${C3_USER_UID}:${C3_USER_GID} ${C3_WORKDIR}

# Configure zsh shell
RUN usermod --shell /usr/bin/zsh ${C3_USER}

# Set up SSH
RUN mkdir -p /var/run/sshd && \
    echo ${C3_USER}:${C3_USER} | chpasswd && \
    echo 'root:root' | chpasswd && \
    sed -i 's/^#PasswordAuthentication yes$/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/^#X11UseLocalhost yes$/X11UseLocalhost no/' /etc/ssh/sshd_config && \
    mkdir -p ${C3_USER_HOME}/.ssh
COPY c3_docker.key.pub ${C3_USER_HOME}/.ssh/authorized_keys
RUN chmod o-rx ${C3_USER_HOME}/.ssh/authorized_keys && \
    chown ${C3_USER_UID}:${C3_USER_GID} ${C3_USER_HOME}/.ssh/authorized_keys
EXPOSE 22

# Bootstrap Simics project
RUN su c3_user -c "$SIMICS_INSTALL_DIR/simics-latest/bin/project-setup --ignore-existing-files ."

CMD ["/usr/sbin/sshd", "-D"]
