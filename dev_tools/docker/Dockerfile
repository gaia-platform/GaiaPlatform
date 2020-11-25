FROM ubuntu:20.04
RUN groupadd -r -g 101 messagebus \
    && groupadd -r -g 102 postgres \
    && groupadd -r -g 103 ssh \
    && groupadd -r -g 104 ssl-cert \
    && groupadd -r -g 105 systemd-timesync \
    && groupadd -r -g 106 systemd-journal \
    && groupadd -r -g 107 systemd-network \
    && groupadd -r -g 108 systemd-resolve \
    && useradd messagebus -l -r -u 101 -g 101 \
    && useradd postgres -l -r -u 102 -g 102 -G ssl-cert \
    && useradd systemd-timesync -l -r -u 103 -g 105 -d /run/systemd \
        -s /usr/sbin/nologin \
    && useradd systemd-network -l -r -u 104 -g 107 -d /run/systemd \
        -s /usr/sbin/nologin \
    && useradd systemd-resolve -l -r -u 105 -g 108 -d /run/systemd \
        -s /usr/sbin/nologin \
    && useradd sshd -l -r -u 106 -d /run/sshd -s /usr/sbin/nologin

RUN echo "APT::Acquire::Retries \"5\";" > /etc/apt/apt.conf.d/80-retries \
    && apt-get update

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        git \
        python3-pip \
        wget \
        flatbuffers-compiler \
        libexplain-dev \
        libstdc++-10-dev \
        openjdk-8-jdk \
        python3-dev \
        clang-tidy \
        clang-format \
        rpm \
        clang-8 \
        git \
        vim \
        libssl-dev \
        make \
    && apt-get clean

# common environment variables
ENV CC=/usr/bin/clang-8
ENV CXX=/usr/bin/clang++-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV LC_CTYPE=en_US.UTF-8

# cmake
RUN mkdir -p /build/third_party/production/cmake
WORKDIR /build/third_party/production/cmake
RUN git clone --depth 1 --branch v3.17.0 https://gitlab.kitware.com/cmake/cmake.git \
    && rm -rf */.git

RUN cd cmake \
    && ./bootstrap \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf *

# backward
RUN mkdir -p /build/third_party/production/backward
WORKDIR /build/third_party/production/backward
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        libdw-dev \
    && apt-get clean

RUN git clone --depth 1 --branch v1.5 https://github.com/bombela/backward-cpp.git \
    && rm -rf */.git

RUN cmake backward-cpp \
    && make -j$(nproc) \
    && make install \
    && rm -rf *

# bison
RUN mkdir -p /build/third_party/production/bison
WORKDIR /build/third_party/production/bison
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        flex \
        m4 \
    && apt-get clean

RUN wget http://ftp.gnu.org/gnu/bison/bison-3.5.tar.gz \
    && apt-get remove --autoremove -y wget

RUN tar -zxvf bison-3.5.tar.gz \
    && cd bison-3.5 \
    && ./configure \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf *

# daemonize
RUN mkdir -p /build/third_party/production/daemonize
WORKDIR /build/third_party/production/daemonize
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        build-essential \
    && apt-get clean

RUN git clone --depth 1 --branch release-1.7.8 https://github.com/bmc/daemonize.git \
    && rm -rf */.git

RUN cd daemonize \
    && ./configure \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf *

# flatcc
RUN mkdir -p /build/third_party/production/flatcc
WORKDIR /build/third_party/production/flatcc
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        ninja-build \
    && apt-get clean

RUN git clone --depth 1 https://github.com/dvidelabs/flatcc.git \
    && rm -rf */.git

RUN cd flatcc \
    && echo "\n\nset_property (TARGET flatccrt PROPERTY POSITION_INDEPENDENT_CODE ON)\n" >> src/runtime/CMakeLists.txt \
    && scripts/build.sh \
    && cp bin/flatcc /usr/local/bin/flatcc \
    && cp -r include/flatcc /usr/local/include/flatcc \
    && cp lib/*.a /usr/local/lib/ \
    && cd .. \
    && rm -rf *

# postgresql
RUN mkdir -p /build/third_party/production/postgresql
WORKDIR /build/third_party/production/postgresql
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        ca-certificates \
        curl \
        gnupg \
        libpq-dev \
        locales \
        lsb-release \
        python3-pip \
        ssl-cert \
    && apt-get clean

RUN curl https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - \
    && echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | tee /etc/apt/sources.list.d/pgdg.list \
    && apt-get update -y \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        postgresql-12 \
        postgresql-12-dbgsym \
        postgresql-client-12 \
        postgresql-client-12-dbgsym \
        postgresql-contrib-12 \
        postgresql-server-dev-12 \
    && apt-get clean \
    && rm /etc/apt/sources.list.d/pgdg.list \
    && python3 -m pip install psycopg2 \
    && locale-gen en_US.UTF-8 \
    && mkdir -p /etc/postgresql/ssl \
    && cp -p -R /etc/ssl/private /etc/postgresql/ssl \
    && sed -i "s/\/etc\/ssl\/private/\/etc\/postgresql\/ssl\/private/g" /etc/postgresql/12/main/postgresql.conf \
    && sed -i '/^local/ s/peer/trust/' /etc/postgresql/12/main/pg_hba.conf

# pybind11
RUN mkdir -p /build/third_party/production/pybind11
WORKDIR /build/third_party/production/pybind11
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        python3 \
        python3-dev \
        python3-pip \
    && apt-get clean

RUN git clone --depth 1 --branch v2.5.0 https://github.com/pybind/pybind11.git \
    && rm -rf */.git

RUN python3 -m pip install pytest \
    && apt-get remove --autoremove -y python3-pip

RUN cmake pybind11 \
    && make -j$(nproc) \
    && make install \
    && rm -rf *

# cpptoml
RUN mkdir -p /build/third_party/production/cpptoml
WORKDIR /build/third_party/production/cpptoml
RUN git clone --depth 1 --branch v0.1.1 https://github.com/skystrife/cpptoml.git \
    && rm -rf */.git

RUN cd cpptoml \
    && rm -fr build/ \
    && cmake -H. -Bbuild  -DCMAKE_BUILD_TYPE=Release \
                       -DCPPTOML_BUILD_EXAMPLES=OFF \
                       -DENABLE_LIBCXX=OFF \
    && cmake --build build --target install \
    && cd .. \
    && rm -rf *

# fmt
RUN mkdir -p /build/third_party/production/fmt
WORKDIR /build/third_party/production/fmt
RUN git clone --depth 1 --branch 7.0.3 https://github.com/fmtlib/fmt.git \
    && rm -rf */.git

RUN cd fmt \
    && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release \
                      -DBUILD_SHARED_LIBS=OFF \
                      -DFMT_DOC=OFF \
                      -DFMT_TEST=OFF \
                      -DFMT_FUZZ=OFF \
                      -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON \
    && cmake --build build --target install \
    && cd .. \
    && rm -rf *

# spdlog
RUN mkdir -p /build/third_party/production/spdlog
WORKDIR /build/third_party/production/spdlog
RUN git clone --depth 1 --branch v1.8.0 https://github.com/gabime/spdlog.git \
    && rm -rf */.git

RUN cd spdlog \
    && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release \
                      -DSPDLOG_BUILD_BENCH=OFF \
                      -DSPDLOG_BUILD_TESTS=OFF \
                      -DSPDLOG_BUILD_EXAMPLE=OFF \
                      -DSPDLOG_FMT_EXTERNAL=ON \
                      -DSPDLOG_BUILD_SHARED=OFF \
                      -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON \
    && cmake --build build --target install \
    && cd .. \
    && rm -rf *

# spdlog_setup
RUN mkdir -p /build/third_party/production/spdlog_setup
WORKDIR /build/third_party/production/spdlog_setup
RUN git clone --depth 1 --branch v0.3.0 https://github.com/guangie88/spdlog_setup.git \
    && rm -rf */.git

RUN cd spdlog_setup \
    && rm -fr build/ \
    && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release \
                      -DCMAKE_THREAD_LIBS_INIT="-lpthread" \
                      -DSPDLOG_SETUP_INCLUDE_UNIT_TESTS=OFF \
                      -DSPDLOG_SETUP_CPPTOML_EXTERNAL=ON \
    && cmake --build build --target install \
    && cd .. \
    && rm -rf *

# tabulate
RUN mkdir -p /build/third_party/production/tabulate
WORKDIR /build/third_party/production/tabulate
RUN git clone --depth 1 --branch v1.3 https://github.com/p-ranav/tabulate \
    && rm -rf */.git

RUN cd tabulate \
    && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target install \
    && cd .. \
    && rm -rf *

RUN echo "LD_PRELOAD = '$(clang-8 -print-file-name=libclang_rt.asan-$(uname -m).so)'" >> /etc/postgresql/12/main/environment \
    && echo "ASAN_OPTIONS = 'detect_leaks=0'" >> /etc/postgresql/12/main/environment

ENV LD_LIBRARY_PATH=/usr/local/lib
RUN mkdir /demos
WORKDIR /demos

ENTRYPOINT [ "/bin/bash" ]
