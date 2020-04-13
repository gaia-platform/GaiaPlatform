FROM ubuntu:18.04

# set these args with `--build-arg` on the `docker build` command line
ARG USER_NAME=dev
ARG USER_PASSWORD=dev
ENV USER_NAME=$USER_NAME
ENV USER_PASSWORD=$USER_PASSWORD

# for zsh customization
ARG ZSH_THEME=pure

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && \
    apt-get -y install ubuntu-dbgsym-keyring && \
    apt-get -y update && \
    apt-get -y upgrade

# install prerequisites
RUN apt-get -y install \
    sudo \
    build-essential \
    libc6-dbg \
    gdb \
    valgrind \
    clang-8 \
    clang-format-8 \
    libclang-8-dev \
    libclang-common-8-dev \
    libclang1-8 \
    uuid-dev \
    git \
    ack-grep \
    emacs \
    curl \
    ca-certificates \
    gnupg \
    lsb-release \
    software-properties-common \
    locales

# alias clang to clang-8
RUN ln -s /usr/bin/clang-8 /usr/local/bin/clang && \
    ln -s /usr/bin/clang++-8 /usr/local/bin/clang++ && \
    ln -s /usr/bin/clang-format-8 /usr/local/bin/clang-format

# use clang not gcc
ENV CC=/usr/bin/clang-8
ENV CXX=/usr/bin/clang++-8

# install script to list Ubuntu debug symbols packages for a binary (for debugging Postgres extensions)
RUN curl 'https://wiki.ubuntu.com/DebuggingProgramCrash?action=AttachFile&do=get&target=list-symbols-packages-v2.sh' > /usr/local/bin/list-symbols-packages && chmod +x /usr/local/bin/list-symbols-packages

# install python3
RUN apt-get -y install \
        python3-pip \
        python3-dev && \
    ln -s /usr/bin/python3 /usr/local/bin/python && \
    pip3 install --upgrade pip

# install pybind11
RUN apt-get -y install \
    python-clang-8 \
    python-pybind11 \
    pybind11-dev

# install latest cmake
RUN curl https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add - && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ `lsb_release -cs` main" && \
    apt-get -y update && \
    apt-get -y install cmake

# set locale properly for zsh and postgres
ENV LC_CTYPE=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
RUN locale-gen en_US.UTF-8 && \
    # add a user (--disabled-password: the user won't be able to use the account until the password is set)
    adduser --quiet --disabled-password --shell /bin/zsh --home /home/$USER_NAME --gecos "User" $USER_NAME && \
    # update the password
    echo "${USER_NAME}:${USER_PASSWORD}" | chpasswd && usermod -aG sudo $USER_NAME && \
    # add to sudoers
    echo "${USER_NAME} ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# install postgres 12
RUN curl https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - && \
    echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | tee /etc/apt/sources.list.d/pgdg.list && \
    apt-get -y update && \
    apt-get -y install \
        postgresql-12 \
        postgresql-12-dbgsym \
        postgresql-client-12 \
        postgresql-client-12-dbgsym \
        postgresql-contrib-12 \
        postgresql-server-dev-12 \
        libpq-dev

# install python postgres client
RUN pip3 install psycopg2

# install preferred flatbuffers version
RUN apt-add-repository ppa:hnakamur/flatbuffers && \
    apt-get -y update && \
    apt-get -y install flatbuffers-compiler>=1.11 && \
    apt-get -y install libflatbuffers-dev>=1.11

# install flatcc (C flatbuffers support)
RUN apt-get -y install ninja-build
RUN git clone https://github.com/dvidelabs/flatcc.git && \
    cd /flatcc && \
    echo "\n\nset_property(TARGET flatccrt PROPERTY POSITION_INDEPENDENT_CODE ON)\n" >> src/runtime/CMakeLists.txt && \
    scripts/build.sh && \
    cp bin/flatcc /usr/local/bin/flatcc && \
    cp -r include/flatcc /usr/local/include/flatcc && \
    cp lib/*.a /usr/local/lib/

# install python flatbuffers package
RUN pip3 install flatbuffers==1.11

# install zsh
RUN apt-get -y install zsh
# now switch to the user for customization
USER $USER_NAME
# terminal colors with xterm
ENV TERM xterm

# Get prezto
RUN git clone --recursive https://github.com/sorin-ionescu/prezto.git ~/.zprezto
# Remove old zsh config
RUN rm -f ~/.zshrc
# Create links to zsh config files
RUN ln -s ~/.zprezto/runcoms/zlogin ~/.zlogin && \
    ln -s ~/.zprezto/runcoms/zlogout ~/.zlogout && \
    ln -s ~/.zprezto/runcoms/zpreztorc ~/.zpreztorc && \
    ln -s ~/.zprezto/runcoms/zprofile ~/.zprofile && \
    ln -s ~/.zprezto/runcoms/zshenv ~/.zshenv && \
    ln -s ~/.zprezto/runcoms/zshrc ~/.zshrc
# set the zsh theme
RUN echo "zstyle ':prezto:module:prompt' theme '$ZSH_THEME'" >> ~/.zpreztorc

# start zsh
CMD ["zsh"]
