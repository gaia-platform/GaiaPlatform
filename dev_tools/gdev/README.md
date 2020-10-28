# `gdev` docker build CLI
Gdev is a command line tool that creates repeatable builds in the GaiaPlatform repo. The builds are
isolated within Docker images and do not depend on any installed packages or configuration on your
host.

## Installation
Symlink `gdev.sh` to your user executable path.
```bash
mkdir -p ~/.local/bin && ln -s $GAIA_REPO/dev_tools/gdev/gdev.sh ~/.local/bin/gdev
```

Ensure your user executable path is in your `PATH` variable. If it is not, add it to your `.bashrc`.
```bash
echo $PATH | grep $HOME/.local/bin \
    || (echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc && source .bashrc)
```
Install Python 3.8 and pip.
* Ubuntu 18.04
    ```bash
    sudo apt-get update && sudo apt-get install -y python3.8 python3-pip
    ```
* Ubuntu 20.04
    ```bash
    sudo apt-get update && sudo apt-get install -y python3-pip
    ```
* Mac
    TODO

Install Python dependencies.
```bash
python3.8 -m pip install --user atools argcomplete
```

Enable gdev tab-completion.
```bash
echo 'eval "$(register-python-argcomplete gdev)"' >> ~/.bashrc && source ~/.bashrc
```

(Linux only) [Install docker for Linux](https://docs.docker.com/engine/install/ubuntu/).
```bash
# It's okay for this command to fail when none of these are installed.
sudo apt-get remove docker docker-engine docker.io containerd runc || :
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg-agent \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get update
sudo apt-get install docker-ce docker-ce-cli containerd.io
```

(Windows only) [Install docker for Windows](https://docs.docker.com/docker-for-windows/wsl/).
(Mac only) [Install docker for Mac](https://docs.docker.com/docker-for-mac/install/).

Perform [docker post-install steps for
Linux](https://docs.docker.com/engine/install/linux-postinstall/).
```bash
sudo groupadd docker
sudo usermod -aG docker $USER
newgrp docker
```
Log out and log back in to have group membership changes take effect.

Enable experimental features and our testing image repo in Docker.
XXX The testing registry uses `http` and is not secure.
* Ubuntu
    In `/etc/docker/daemon.json`, add the experimental field.
    ```
    {
    "experimental": true,
    "insecure-registries": ["192.168.0.250:5000"]
    }
    ```
    In `~/.docker/config.json`, add the experimental field.
    ```
    {
    "experimental": "enabled"
    }
    ```
    Restart the docker daemon to make changes take effect.
    ```bash
    sudo service docker restart
    ```
* Mac
    In the "Docker Desktop" menu, go to `Preferences > Command Line` and turn on `Enable
    Experimental Features`.

    Click `Apply and Restart` to make the changes take effect.

(Optional, Linux only) Enable multiarch building.
```bash
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
docker buildx create --name gdev_builder
docker buildx use gdev_builder
docker buildx inspect --bootstrap
```

## Creating build rules with `gdev.cfg` files
`gdev.cfg` files are simple configuration files. They belong in the same directory as the associated
code that they describe how to build.
```
GaiaPlatform (aka <repo_root>)
+- production
|  |- gdev.cfg  # Describes how to build production.
|  |- CMakeLists.txt
|  |- ...
|
+- third_party
|  +- production
|  |  +- bison
|  |  |  |- gdev.cfg  # Describes how to clone, build, and install bison.
|  |  |
|  |  +- cmake
|  |  |  |- gdev.cfg  # Describes how to clone, build, and install cmake.
|  |  |
|  |  +- postgresql
|  |  |  |- gdev.cfg  # Describes how to install postgresql from a third-party apt repo.
|  |  |
|  |  +- pybind11
|  |  |  |- gdev.cfg  # Describes how to clone, build, and install pybind11.
|  |  |
|  |  +- ...
|  |
|  +- ...
|
+- demos
|  +- incubator
|  |  |- gdev.cfg
|  |  |- ...
|  |
|  +- ...
|
+- ...
```

This is an example of the `<repo_root>/third_party/production/cmake/gdev.cfg` contents.
```
[apt]
clang-8
git
libssl-dev
make

[env]
CC=/usr/bin/clang-8
CXX=/usr/bin/clang++-8

[git]
--branch v3.17.0 https://gitlab.kitware.com/cmake/cmake.git

[run]
cd cmake
./bootstrap
make -j$(nproc)
make install
```

... and an example of the `<repo_root>/production/gdev.cfg` contents.
```
[apt]
flatbuffers-compiler
libexplain-dev
libstdc++-10-dev
openjdk-8-jdk
python3-dev

[gaia]
production/cmake
production/sql
third_party/production/bison
third_party/production/cmake
third_party/production/flatcc
third_party/production/googletest
third_party/production/postgresql
third_party/production/pybind11
third_party/production/rocksdb

[run]
cmake -DCMAKE_MODULE_PATH={source_dir('production/cmake')} \
    {source_dir('production')}
make -j$(nproc)
```
Note that `third_party/production/cmake` is listed as a `[gaia]` dependency. When using `gdev` to
build production, the dependencies in the `[gaia]` section are built first and the associated build
outputs are made available while building production.

`gdev.cfg` files contain the following sections.
* `[apt]`
    List of apt dependencies to install. One dependency per line.
* `[env]`
    List of environment variables to set. One environment variable per line.
* `[gaia]`
    List of gaia dependencies to include. Dependencies are paths relative to the GaiaPlatform
    <repo_root>, e.g.  `production`, `third_party/production/cmake`.  Dependencies may have a
    `gdev.cfg` file to define build rules. Any such build rules from gaia dependencies will be built
    before the current build rule and the source code and build output will be available to the
    current build target. The the source directory of the dependency relative to <repo_root> will be
    available in the image.  One dependency per line.
* `[git]`
    List of git repos to clone into the build folder of the Docker image. One dependency per line.
* `[pip]`
    List of python3 pip dependencies to install. One dependency per line.
* `[web]`
    List of web dependencies to download into build folder. One dependency per line.
* `[pre_run]`
    List of commands to run before the main run. Commands are run in the build folder in the Docker
    image for the current build rule. One command per line. The dependencies defined in `[apt]`,
    `[env]`, `[gaia]`, `[git]`, `[pip]`, and `[web]` sections become available for use in this
    section.
* `[run]`
    List of commands to run after pre_run. This is the final stage for the build rule. Commands are
    run in the build folder in the Docker image for the current build rule. One command per line.

### Docker image `/source` and `/build`
Source code from `<repo_root>` is copied to the Docker image in the `[pre_run]` step. In the Docker
image, <repo_root> corresponds to `/source`. `/build` mirrors the directory structure of `/source`,
but is where we build. So, after building `<repo_root>/production`, the source code will have been
copied to `/source/production` and the build artifacts will be at `/build/production`.

Note that during image build, not all of the GaiaPlatform repo is copied into the Docker image. Only
the directory subtree of the current build rule (e.g. `<repo_root>/production/*`) and the build
rules for transitive `[gaia]` dependencies (e.g. `<repo_root>/third_party/production/postgresql`)
are copied into the image. This makes it possible for Docker to cache build stages based just on
file contents of the directory subtrees and not have to rebuild them all the time. So, modifying
code in the `<repo_root>/production` directory subtree will not cause docker to rebuild anything in
`<repo_root>/third_party/...`.

There are two Python string replacements available for use in any section of the `gdev.cfg` files to
aid in ensuring you use specify the correct directories in the Docker image.
1. `{source_dir('some directory')}`
    This translates to the source directory for the target in the docker image, e.g.
    `{source_dir('production')}` translates to `/source/production` and
    `{source_dir('third_party/production/cmake')}` translates
    `/source/third_party/production/cmake`
1. `{build_dir('some directory')}`
    Similar to `{source_dir()}`, this translates to the build directory for the target in the
    docker image, e.g.  `{build_dir('production')}` translates to `/build/production` and
    `{build_dir('third_party/production/cmake')}` translates to
    `/build/third_party/production/cmake`

## `gdev` CLI Usage
The directory in which you execute `gdev` commands is the gdev context. If you `gdev build` in
`<repo_root>/production`, you will build production.

### Top-level commands
* `gdev cfg`
    Parse the contents of the `gdev.cfg` file in the current directory.
* `gdev dockerfile`
    Given the contents of the `gdev.cfg` file (automatic via `gdev cfg`), creates a Dockerfile that
    will build a docker image.
* `gdev build`
    Given the generated Dockerfile (automatic via `gdev dockerfile`), builds a Docker image using
    the root of the GaiaPlatform repo as the build context.
* `gdev run`
    Given the built Docker image (automatic via `gdev build`), runs a command (`/bin/bash` if no
    command given) in an ephemeral Docker container.
* `gdev gen`
    This provides a CLI entrypoint to all the underlying commands used for the other top-level
    commands. This should rarely be used.

Note that all of these commands may be run without running dependent commands, i.e. `gdev run` which
depends on `gdev build` which depends on `gdev dockerfile`. Each command will invoke dependent
commands by itself.

### Building
In normal workflow, `gdev` only tracks images with the `latest` tag. Whenever the image is built, we
also record metadata about what git hash it was built with and whether it was "tainted" with
uncommitted changes or untracked files.
```bash
cameron@host:~/GaiaPlatform/production$ gdev build
```

Once gdev builds an image with your current git hash, subsequent calls to `gdev build` will see that
the image is already exists and will not try to rebuild it. If you have local changes that need to
be built into the image, but your current git hash hasn't changed, you can force the changes into
the image by providing the `--force` (or equivalently the `-f`) flag.
```bash
cameron@host:~/GaiaPlatform/production$ gdev build -f
```

### Running programs
You can run programs in the built docker image. Note that running programs will run `gdev build` if
the correct docker image is not already built.
```bash
# Runs ctest in an ephemeral container and then exits.
cameron@host:~/GaiaPlatform/production$ gdev run ctest
```

If you give no arguments to `gdev run`, it drops you into a bash shell.
```bash
# Runs bash in an ephemeral container.
cameron@host:~/GaiaPlatform/production$ gdev run
cameron@user__production__run:/build/production$
```
Note that the user in the container is based on *you* from your host. You have passwordless root
privileges inside the container. Also, the root of your `GaiaPlatform` repo is mounted into the
container at `/source`. You may make code and configuration edits to the code on your host and
rebuild within the container.

Useful programs that are not normally included in the build can be *mixed* into the image and then
run. The `--sticky-mixins` flag is *sticky* (the value you provide is saved until you change it).
```bash
cameron@host:~/GaiaPlatform/production$ gdev run --sticky-mixins gdb sudo
cameron@gdb__user__production__run:/build/production$ gdb
...snip...
(gdb)
```
The `gdb` mixin is created with the `<repo_root>/dev_tools/gdev/mixin/gdb/gdev.cfg` file, with these
contents.
```text
[apt]
gdb
```
You can see the available mixins with `gdev run --help`.

You can also mount directories from your host to the container or from your container to your host
with the `--sticky-mounts` flag. You can provide a list of `<host_path>:<container_path>` mappings
to bind. By default, `gdev run` mounts the container's current working directory folder to your host
at `./build.gdev` (equivalent to specifying `--sticky-mixins ./build.gdev:.`). Note that the mounts
are persistent and are not ephemeral like the containers. When a container is cleaned up, the
mounted folders remain on the host.
```bash
cameron@host:~/GaiaPlatform/production$ gdev run ctest  -T Test --no-compress-output
cameron@host:~/GaiaPlatform/production$ ls build.gdev/
...snip contents of the mounted build directory...
```
`--sticky-mounts <host_path>:<container_path>` behavior follows the following rules in order.
1. If the `<host_path>` directory exists on the host, the host folder will be mounted into the
container at `<container_path>`.
2. If the `<host_path>` directory does not exist on the host, the container folder at
`<container_path` will be mounted into the host at `<host_path>`.
3. If neither the `<host_path>` nor the `<container_path>` exists, an empty folder will be created
and will be mounted.

## Best-practices
TODO

## Multi-stage build and merge process
TODO

# Examples

## enable_if and enable_if_not

{enable_if('translation_engine')}# This is an example line of what could be run \
    # if we use `gdev build --cfg-enables translation_engine`.
{enable_if_not('translation_engine')}# This is an example line of what could be \
    # if we use `gdev build`, but don't specify `--cfg-includes \
    # translation_engine`.

# Note that the `[run]` section above is the first valid `[run]` section that
# the cfg parser will evaluate, so it is the one that is always used. The
# following examples of gated `[run]` sections are shadowed by the one above.

{enable_if('translation_engine')}[run]
# alternatively, you could have an entire `[run]` section gated with the above \
# line. If you do so, you should also gate the default `[run]` section with \
# `enable_if_not('translation_engine')`.
{enable_if_not('translation_engine')}[run]

## enable_if_any, enable_if_not_any, enable_if_all, enable_if_not_all

# Similar the enable_if and enable_if_not, but multiple keywords may be
# specified. A good example would be
cmake \
    {enable_if_not_any('debug', 'production_debug')}-DCMAKE_BUILD_TYPE=Release \
    {enable_if_any('debug', 'production_debug')}-DCMAKE_BUILD_TYPE=Debug \
    {source_dir('production')}
