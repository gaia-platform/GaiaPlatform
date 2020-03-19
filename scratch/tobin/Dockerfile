FROM ubuntu:18.04

# set these args with `--build-arg` on the `docker build` command line
ARG USER_NAME=dev
ARG USER_PASSWORD=dev
ENV USER_NAME=$USER_NAME
ENV USER_PASSWORD=$USER_PASSWORD

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y upgrade

# install prerequisites
RUN apt-get -y install \
    sudo \
    build-essential \
    clang-8 \
    libclang-8-dev \
    libclang-common-8-dev \
    libclang1-8 \
    cmake \
    git \
    curl \
    ca-certificates \
    gnupg \
    lsb-release \
    locales

# use clang not gcc
ENV CC=/usr/bin/clang-8
ENV CXX=/usr/bin/clang++-8

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

# set locale properly for zsh and postgres
ENV LC_CTYPE=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
RUN locale-gen en_US.UTF-8 && \
# add a user (--disabled-password: the usedr won't be able to use the account until the password is set)
adduser --quiet --disabled-password --shell /bin/zsh --home /home/$USER_NAME --gecos "User" $USER_NAME && \
# update the password
echo "${USER_NAME}:${USER_PASSWORD}" | chpasswd && usermod -aG sudo $USER_NAME && \
# add to sudoers
echo "${USER_NAME} ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# install postgres 12
RUN curl https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - && \
echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | tee /etc/apt/sources.list.d/pgdg.list && \
apt-get -y update
RUN apt-get -y install \
postgresql-12 \
postgresql-client-12 \
postgresql-contrib-12 \
postgresql-server-dev-12 \
libpq-dev

# install python postgres client
RUN pip3 install psycopg2

# install zsh
RUN apt-get -y install zsh
# now switch to the user for customization
USER $USER_NAME
# terminal colors with xterm
ENV TERM xterm
# set the zsh theme
ENV ZSH_THEME agnoster
# install oh-my-zsh for this user
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
# start zsh
CMD ["zsh"]
