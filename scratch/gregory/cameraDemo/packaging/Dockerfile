FROM ubuntu:18.04

# Stop apt from prompting us for info such as location or time zone.
ENV DEBIAN_FRONTEND=noninteractive

# Create AppDir folders. Everything in the AppDir will be included in the final executable.
ARG APP_DIR=/AppDir
ARG USR_DIR=${APP_DIR}/usr
ARG BIN_DIR=${USR_DIR}/bin
ARG LIB_DIR=${USR_DIR}/lib
RUN mkdir -p ${APP_DIR} ${USR_DIR} ${BIN_DIR} ${LIB_DIR}

# Install cmake suitable for Gaia Platform.
RUN apt-get -y update \
    && apt-get install -y \
        apt-transport-https \
        ca-certificates \
        locales \
        lsb-release \
        software-properties-common \
        wget \
    && wget -O- https://apt.kitware.com/keys/kitware-archive-latest.asc \
        | apt-key add - \
    && apt-add-repository "deb https://apt.kitware.com/ubuntu/ `lsb_release -cs` main" \
    && apt-get -y update \
    && apt-get -y install cmake

# Get OpenCV and supplementary repos.
ARG OPENCV_VERSION=4.3.0
RUN apt-get install -y git \
    && git clone --depth 1 --branch ${OPENCV_VERSION} https://github.com/opencv/opencv.git \
    && git clone --depth 1 --branch ${OPENCV_VERSION} https://github.com/opencv/opencv_contrib.git \
    && git clone --depth 1 --branch ${OPENCV_VERSION} https://github.com/opencv/opencv_extra.git

# Install OpenCV build and run dependencies.
RUN apt-get install -y \
        # Build depencencies.
        build-essential \
        # cmake \ # Already installed. \
        pkg-config \
        # Run dependencies.
        libgtk-3-dev \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libv4l-dev \
        libxvidcore-dev \
        libx264-dev \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        gfortran \
        openexr \
        libatlas-base-dev \
        python-dev \
        python-numpy \
        python3-dev \
        python3-numpy \
        libtbb2 \
        libtbb-dev \
        libdc1394-22-dev \
        libssl-dev

# Build and install OpenCV.
WORKDIR /opencv/build
RUN cmake -D CMAKE_BUILD_TYPE=RELEASE \
        -D INSTALL_C_EXAMPLES=ON \
        -D INSTALL_PYTHON_EXAMPLES=ON \
        -D OPENCV_GENERATE_PKGCONFIG=ON \
        -D OPENCV_EXTRA_MODULES_PATH=/opencv_contrib/modules \
        -D BUILD_EXAMPLES=ON .. \
    && make -j$(nproc) \
    && make install

# Get OpenCV ML model data.
WORKDIR ${BIN_DIR}
RUN wget https://pjreddie.com/media/files/yolov3.weights \
    && cp /opencv/samples/data/dnn/object_detection_classes_yolov3.txt . \
    && cp /opencv_extra/testdata/dnn/yolov3.cfg .

# Get the AppImage that will bundle our final AppDir.
ARG LINUX_DEPLOY=/linuxdeploy.AppImage
RUN wget -O ${LINUX_DEPLOY} https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-$(arch).AppImage \
    && chmod +x ${LINUX_DEPLOY}

# Get the standard AppRun, the entrypoint of our final AppImage. It does some convenient
# environment and cwd setup so that our "<AppDir>/lib" is part of our LD_LIBRARY_PATH.
ARG APP_RUN=${APP_DIR}/AppRun
RUN wget -O ${APP_RUN} https://github.com/AppImage/AppImageKit/releases/download/continuous/AppRun-$(arch) \
    && chmod +x ${APP_RUN}

# Install Gaia Platform production build dependencies.
RUN apt-get install -y \
        clang-8 \
        pybind11-dev

# Build Gaia Platform production.
COPY /production /GaiaPlatform/production
COPY /third_party /GaiaPlatform/third_party
ENV CC=/usr/bin/clang-8
ENV CXX=/usr/bin/clang++-8
WORKDIR /GaiaPlatform/production/build
RUN rm -rf * \
    && cmake .. \
    && make -j$(nproc)

# Build cameraDemo.
COPY /scratch/gregory/cameraDemo /GaiaPlatform/scratch/gregory/cameraDemo
WORKDIR /GaiaPlatform/scratch/gregory/cameraDemo/build
RUN rm -rf * \
    && cmake .. \
    && make -j$(nproc)

# Copy static AppImage metadata, icon, and our actual binary to the expected paths in our AppDir.
WORKDIR /GaiaPlatform/scratch/gregory/cameraDemo
RUN cp packaging/camera_demo.desktop ${APP_DIR} \
    && cp packaging/camera_demo.png ${APP_DIR} \
    && cp build/camera_demo ${BIN_DIR}

# Include all the shared libraries camera_demo depends upon in the AppDir.
WORKDIR ${BIN_DIR}
RUN ldd camera_demo \
        | grep -oP ".* => \K.*\.so(\.[\d]+)*" \
        | xargs cp -t ${LIB_DIR}

# Remove shared objects that are know to exist on all linux distros.
WORKDIR ${LIB_DIR}
RUN wget -O- https://raw.githubusercontent.com/AppImage/pkg2appimage/master/excludelist \
        | grep -P "^[^# ]+$" \
        | xargs rm -f

# Fix all instances if "/usr/..." in our packaged binary and shared libraries. The CWD while the
# packaged application runs will be at "<mountpoint>/AppDir/", so these expected library paths
# need to be changed from "/usr/..." to "./usr/...".
WORKDIR ${USR_DIR}
RUN find . -type f -exec sed -i -e 's#/usr#././#g' {} \;

# Build our final AppImage. We have to extract the linux deploy appimage and execute its internal
# AppRun. We can't just execute the AppImage since it can't self-mount a fuse filesystem inside a
# docker build context.
WORKDIR /
RUN ${LINUX_DEPLOY} --appimage-extract-and-run --appdir ${APP_DIR}

ENTRYPOINT ["/bin/bash"]
