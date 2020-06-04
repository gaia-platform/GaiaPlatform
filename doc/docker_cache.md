# Docker Build Environment
## Goal
Fast, repeatable builds that just work.

## Terminology
- target - Corresponds to a folder in the GaiaPlatform git repo. Usually has a CMakeLists.txt
  file that builds some output that we depend upon.
- dockerfile - A file with instructions to build a docker image. In our case, contains the
  sequence of commands that installs dependencies and build one of our targets.
- image - A docker image. The output of the `docker build` command. It can serve as the base
  layer to build upon for another image or as the starting point for a container.
- container - an ephemeral, contained environment based on an image where mutation is allowed.
  Usually runs a single process such as bash or gdb and then is cleaned up. The container uses
  the host kernel and host resources like a normal process. It does not require resource
  reservations or virtualization (in normal use-cases).
- volume - A shared folder between a container and the host.

## Tools
- Docker
- Python3

## Installing Dependencies
For a fully-automatable build system, dependency installion needs to be scriptable. Ideally, it
should also be easy to do manually. Using \*requirements\*.txt files should be simple and
satisfactory. The idea for these files is that they can be used with `xargs` or with `cat file |
installer`. If this creates too big a mess of files, we could switch to a `requirements.cfg` like
```
[apt]
dep0
dep1

[pip]
dep3>=4.2

[wget]
https://dep4

[gaia]
/production
/third-part/production/opencv
```

### apt_requirement.txt

For any apt dependencies. Install with
```
xargs -a apt_requirements.txt apt-get install -y
```

### pip_requirements.txt
Required for any pip dependencies. File may look like
```
numpy
pybind11>=2.2
```
Install with
```
xargs -a pip_requirements.txt python3 -m pip install -y
````

### gaia_requirements.txt

This is required for internal project dependencies or internal third_party builds.

The file has one gaia target per line. Each path leads to a GaiaPlatform directory either
relative to the repo root or to `.` that needs to be built and installed.

For example, the `/production/gaia_requirements.txt` file may look like
```
./db
./direct_access
./rules
./sql
./system
/third_party/production/flatbuffers
/third_party/production/googletest
/third_party/production/rocksdb
```

The `/scratch/gregory/cameraDemo/gaia_requirements.txt` file may look like
```
/production@tags/q1  # Notice the branch specification.
/third_party/opencv
```

Note that there is no plan to make this work with branches already in the past like `@tags/q1` since
it would be difficult and would provide little benefit.

Each line implicitly indicates the current HEAD commit unless specified otherwise. The line may
be followed by the last working branch name (like tags/q1) or commit hash before an API change in
the dependency was not followed by the dependee. The gaia target should be built at that commit
hash or git tag.

Note that it would be preferable to always keep projects up-to-date so that they build with the
latest commit of other gaia_requirements.txt. Usage of `@<branch>` and `@<hash>` should be used
sparingly and with the intent that the build target will be updated to work with the latest code.
The feature will exist to allow us to iterate on dependees (like /production) without the burden
of immediately updating dependents (like /demos).

### wget_requirements.txt
For any raw artifacts that need to be downloaded.

```
xargs -a wget_requirements.txt wget
```

For example, in `/third_party/evaluation/opencv/wget_requirements.txt`
```
https://pjreddie.com/media/files/yolov3.weights
https://raw.githubusercontent.com/opencv/opencv/4.3.0/samples/data/dnn/object_detection_classes_yolov3.txt
https://raw.githubusercontent.com/opencv/opencv_extra/4.3.0/testdata/dnn/yolov3.cfg
```

### Specific OS Release or CPU Architecture
In the future, there may be need to support different Linux releases and cpu architectures.
Specific needs for specific environments can be placed in files such as
`apt_requirements_focal_arm64v8.txt` or `pip_requirements_bionic_amd64.txt`. Generic needs for
all environments still go in files such as `apt_requirements.txt`.

Initially, we'll only build for bionic on amd64.

### build.sh
For custom build command. This should usually not be needed. If missing, we just run 
```
mkdir -p build
cd build
cmake ..
make -j$(ncpu)
```
Otherwise, invoke `./build.sh`

For example, in `/third_party/evaluation/opencv/build.sh`
```
git clone --depth 1 --branch 4.3.0 https://github.com/opencv/opencv.git
git clone --depth 1 --branch 4.3.0 https://github.com/opencv/opencv_contrib.git
mkdir -p opencv/build
cd opencv/build
cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..
make -j$(nproc)
```

## Changes to our CMakeLists.txt
The proposed docker system mostly works with our current cmake build environment. We need to make
one change.

### The problem with our cmake system
Our dependees (such as the camera demo) are currently able to include header files of dependencies
using relative paths in the source tree. Camera demo has a dependency of `/production@tags/q1`.
The production in the current checked out repo is `/production@HEAD`. The relative path in our
cmake, `../../../../production/inc` leads to newer headers than we want.

### Suggested improvements to our cmake system
- We can use more generic cmake mechanisms for including our dependencies without relying on
  paths. We would only be using `add_library` and `target_link_library`. For that to happen,
  we'd need to either install our libraries (in the docker image, not our hosts) with headers
  or use the cmake `export` function to register the public libraries with our cmake system.

- We can require parameter inputs for library locations. So, when building camera demo we could
  provide `-DGAIA_PROD_BUILD=<path to prod build in a tags/q1 checkout>`. I'm less a fan of this
  since it makes our `cmake` invocations more complicated and harder to automate.

## Docker
Docker is a fantastic tool for creating repeatable builds. Unlike with virtual machines,
processes running in docker have near-zero overhead when used in native Linux or with WSL2
(docker for Mac uses a Linux virtual machine to run containers and has significant overhead and
resource constraints).

### Where do gaia dependencies built in docker go on the host?
Docker allows us to bind directories from a container to the host as volume. If you want to use
the built artifacts from a docker image to populate build folders on your host, you may do so
with `gdev mount`. The build directory for your current target will be mounted at its correct
place in the source tree on your host.

### Where do apt/pip dependencies go on the host?
If you had installed these dependencies on your host and not just in a docker image, these would
live in your host's `/usr` folder, but we won't pollute that. We'll bind the container's `/usr`
to reasonable place like `.../build/.local/usr`. We'll add the path to your `LD_LIBRARY_PATH`. We
may also add `.../build/.local/usr/bin` to your `PATH`.

### Can we just take build artifacts from an image and build upon them from our host?
Probably yes, though I recommend avoiding this. Your mutable host build environment is way harder
to debug than the immutable container build environment.

As for how we can do this, we can copy all the build folders from a docker image to their places
in your current source tree. We'd also need to register those build directories with the cmake
registry. I haven't prototyped this so I'm not certain I know all the work required.

## Hosting Images
Initially, we'll host docker images on our build machine. This means you'll need to connect to the
VPN to connect to the docker image repository. If the build maching handles our traffic well enough,
we can try to add a port forward in our firewall to make VPN unnecessary.

Docker provides an image for that hosts an image repository. It requires little setup. We'll try
this first.

## Tagging Images
We'll name images as follows.

`<project name>:<tag>`

The image name will correspond to exported project names in our repo (like production, gaia_llvm,
gaia_direct). We'll have multiple tags including
- Branch name (if exists)
- Git hash
- latest (same as master, but this is a special default  tag name in docker)

## gdev.py - a CLI to make our workflow easier
Note that I'm not settled on the workflow, but this shows what it could be like.

### Building Images
Images are build with 
```
gdev build [target]
```

### Running Images
Images are run with 
```
gdev run [target]
```

### Inspecting image environment
You can drop into a container bash shell that has the entire build environment set up with
```
gdev shell [target]
```

### Expiring Images
Images are expired (deleted from the main image repo) with 
```
gdev expire [target] --older-than 90d
```

This won't be necessary for a while, but eventually we need to evict images. We'll never delete
an image that has a tag with a still-existing git branch name or is a dependency for another
unexpired image. The docker cli has good tools for this workflow, so it shouldn't be hard to
automate when we need it.

## Sample workflow - Building inside Docker container
If I want to build the camera demo in docker, I would benefit greatly from using cached
dependencies. OpenCV takes nearly 1 hour with 100% of my system resources to compile.
Alternatively, a cached copy of ~100MB of build artifacts can be extracted from an image that I
can download from our build machine.
```
$ git clone git@github.com:gaia-platform/GaiaPlatform.git

$ cd ~/GaiaPlatform/scratch/gregory/cameraDemo

# Pull all docker images that camera demo depends upon at the current commit hash and mount the
# container's `/usr` to `.../build/.local/usr`.
Then, drop down into a shell with dependencies in place.
cameron@host:~/GaiaPlatform/scratch/gregory/cameraDemo$ ~/GaiaPlatform/production/tools/gdev.py shell

cameron@container:~/GaiaPlatform/scratch/gregory/cameraDemo$ cd build

cameron@container:~/GaiaPlatform/scratch/gregory/cameraDemo$ cmake ..

# Repeat the next two steps while developing.
cameron@container:~/GaiaPlatform/scratch/gregory/cameraDemo$ make -j$(ncpu)

cameron@container:~/GaiaPlatform/scratch/gregory/cameraDemo$ ./camera_demo
```

## Sample workflow - Building outside Docker container
```
$ git clone git@github.com:gaia-platform/GaiaPlatform.git

$ cd ~/GaiaPlatform/scratch/gregory/cameraDemo

# Pull all docker images that camera demo depends upon at the current commit hash and mount the
# container's `/usr` to `.../build/.local/usr` Then, prepends
# `~/GaiaPlatform/scratch/gregory/cameraDemo/build/.local/usr/lib` to `LD_LIBRARY_PATH` and
# `~/GaiaPlatform/scratch/gregory/cameraDemo/build/.local/usr/bin` to `PATH`.
# Also from the container, bind each built GaiaPlatform/.../build directory to the corresponding
# GaiaPlatform/.../build directory on the host. Copy the cmake package registry from the container
# to the host.
cameron@host:~/GaiaPlatform/scratch/gregory/cameraDemo$ ~/GaiaPlatform/production/tools/gdev.py init

cameron@host:~/GaiaPlatform/scratch/gregory/cameraDemo$ cd build

cameron@host:~/GaiaPlatform/scratch/gregory/cameraDemo$ cmake ..

# Repeat the next two steps while developing.
cameron@host:~/GaiaPlatform/scratch/gregory/cameraDemo$ make -j$(ncpu)

cameron@host:~/GaiaPlatform/scratch/gregory/cameraDemo$ ./camera_demo
```

Note that I haven't ensured that this works and there's some change I'll find that it's not possible
or just really difficult and brittle.

## Answers to Questions I'd Ask
### Why no docker commands in sample workflows?
Docker is an extremely powerful tool, but almost everything requires a flag. Everyone here would
need to go through a non-trivial learning process to get it all right, and then we'd be left
using moderately complex, error-prone docker commands. Wrapping it would create a much easier
build environment.

### Will gdev.py or docker be required for building?
No, absolutely not. We can still build with cmake from our hosts. These new tools and practices
will just make it easier and faster. In fact, some of the things we need to improve
(requirements.txt, export in CMakeLists.txt) will make the pure cmake workflow easier as well.

### Can I use GDB with programs running in the container?
Yes. There are some flags required to make it work and gdb needs to be installed inside the
container. It's one of the things that `gdev.py` and our build system would put in place.

### Can I build inside the container from an IDE?
Yes. We'll have a wiki describing how to set it up.