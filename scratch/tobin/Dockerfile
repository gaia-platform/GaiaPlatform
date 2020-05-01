FROM ubuntu:18.04

# set these args with `--build-arg` on the `docker build` command line
ARG USER_NAME=dev
ARG USER_PASSWORD=dev
ENV USER_NAME=$USER_NAME
ENV USER_PASSWORD=$USER_PASSWORD

# for zsh customization
ARG ZSH_THEME=pure
# TinkerPop/gremlin version
ARG GREMLIN_VERSION=3.4.6

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
    openjdk-8-jdk \
    uuid-dev \
    git \
    ack-grep \
    emacs \
    xxd \
    curl \
    unzip \
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

# install python2 (for pybind11)
RUN apt-get -y install \
        python-pip \
        python-dev && \
    pip install --upgrade pip

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

# set up build dir in container fs (to avoid building in mounted source tree)
RUN mkdir /build && chmod 777 build

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

# change local postgres auth to trusted
RUN sed -i '/^local/ s/peer/trust/' `pg_conftool -s 12 main show hba_file`

# install python postgres client
RUN pip install psycopg2

# install gremlin server
RUN GREMLIN_SERVER_DIR="apache-tinkerpop-gremlin-server-${GREMLIN_VERSION}" && \
    GREMLIN_SERVER_FILE="${GREMLIN_SERVER_DIR}-bin.zip" && \
    GREMLIN_SERVER_URL="https://archive.apache.org/dist/tinkerpop/${GREMLIN_VERSION}/${GREMLIN_SERVER_FILE}" && \
    GREMLIN_SERVER_INSTALL_DIR="/usr/local/share/gremlin-server" && \
    curl "$GREMLIN_SERVER_URL" > "$GREMLIN_SERVER_FILE" && \
    unzip "$GREMLIN_SERVER_FILE" && \
    mv "$GREMLIN_SERVER_DIR" "$GREMLIN_SERVER_INSTALL_DIR"
# install gremlin console
RUN GREMLIN_CONSOLE_DIR="apache-tinkerpop-gremlin-console-${GREMLIN_VERSION}" && \
    GREMLIN_CONSOLE_FILE="${GREMLIN_CONSOLE_DIR}-bin.zip" && \
    GREMLIN_CONSOLE_URL="https://archive.apache.org/dist/tinkerpop/${GREMLIN_VERSION}/${GREMLIN_CONSOLE_FILE}" && \
    GREMLIN_CONSOLE_INSTALL_DIR="/usr/local/share/gremlin-console" && \
    curl "$GREMLIN_CONSOLE_URL" > "$GREMLIN_CONSOLE_FILE" && \
    unzip "$GREMLIN_CONSOLE_FILE" && \
    mv "$GREMLIN_CONSOLE_DIR" "$GREMLIN_CONSOLE_INSTALL_DIR"

# install flatcc (C flatbuffers support)
RUN apt-get -y install ninja-build
RUN git clone https://github.com/dvidelabs/flatcc.git && \
    cd /flatcc && \
    # we have to compile flatcc builder runtime static lib with -fPIC so it can be linked with a shared lib
    echo "\n\nset_property (TARGET flatccrt PROPERTY POSITION_INDEPENDENT_CODE ON)\n" >> src/runtime/CMakeLists.txt && \
    scripts/build.sh && \
    cp bin/flatcc /usr/local/bin/flatcc && \
    cp -r include/flatcc /usr/local/include/flatcc && \
    cp lib/*.a /usr/local/lib/

# install python flatbuffers package
RUN pip install flatbuffers==1.11

# install ripgrep (fast file searching)
RUN add-apt-repository ppa:x4121/ripgrep && \
    apt-get -y update && \
    apt-get -y install ripgrep

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

# set build alias
RUN echo "alias build='cmake -S /gaia-platform/production -B /build -DCMAKE_BUILD_TYPE=Debug -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python -G \"Unix Makefiles\" && pushd /build; make && sudo make install && make test; popd'" >> ~/.zshrc

# start zsh
CMD ["zsh"]
