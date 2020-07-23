# `gdev` docker build CLI
Gdev is a command line tool that creates repeatable builds in the GaiaPlatform repo. The builds are
isolated within Docker images and do not depend on any installed packages or configuration on your
host.

## Installation
Install `gdev` dependencies.
```bash
sudo apt-get update
sudo apt-get install python3.8 python3-pip
python3.8 -m pip install --user atools argcomplete
```

Add gdev alias to your `.bashrc`. Note that you need to **set the `$GAIA_REPO`
environment variable** for this to work.
```bash
echo "alias gdev=\"PYTHONPATH=$GAIA_REPO/dev_tools/gdev python3.8 -m gdev\"" >> ~/.bashrc
```

Enable experimental features in Docker by editing the `/etc/docker/daemon.json` file.
```bash
sudo vi /etc/docker/daemon.json
```
Ensure the `/etc/docker/daemon.json` file contents match the following and save.
```
{
"experimental": true
}
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
Then, log out and log back in to have group membership changes take effect.

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
    Note that dependencies defined in `[apt]`, `[env]`, `[gaia]`, `[git]`, and `[pip]` sections
    become available for use in this section.  List of commands to run before the main run. Commands
    are run in the build folder in the Docker image for the current build rule. One command per line.
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
Internally, `gdev` tags docker images using git hashes. An image tagged with a git can contain only
what was in the git commit and nothing else.
```bash
# Ok
cameron@host:~/GaiaPlatform/production$ gdev build
```

 If you try to build for the first time with local changes in the directory subtree of a build
target or one of its transitive dependencies, `gdev` will refuse to build.
```bash
# Not ok. `gdev` refuses to build with uncommitted changes in the `production` subtree.
cameron@host:~/GaiaPlatform/production$ echo "this taints the image!" > foo.txt
cameron@host:~/GaiaPlatform/production$ gdev build
```
However, modifications that around outside the directory subtree of the build target and its
transitive dependencies will not taint the image as the file will never be copied into the image.
```bash
# Ok. Neither `production` nor its transitive dependencies use the `production/scratch` subtree.
cameron@host:~/GaiaPlatform/production$ echo "this does not taint the image!" > ../scratch/foo.txt
cameron@host:~/GaiaPlatform/production$ gdev build
```
We do this for two reasons.
1. Checking `docker image ls <name>:<tag>` to see if our image is already built is an extremely fast
   operation. It is very useful to know that an image named `<name>:<tag>` has exactly the contents
   we mean it to have.
2. Internally, the docker image has a build cache with keys calculated based on the contents of what
   we copy into the image. If files change, so do the build cache keys. While building an image, it
   is possible to query against a remote Docker image repository that has images built. Those remote
   images need to be build with the correct cache keys (meaning no file modifications) for this to
   work. Locally, we need to guarantee the same thing so that we generate the matching cache key as
   the remote image.

If you must build an image locally with uncommitted changes, you can build a tainted image. Note
that you must use `--tainted` for subsequent commands such as `gdev run --tainted` to actually use
the tainted image. Every time you use the `--tainted` flag, docker will re-evaluate whether it needs
to rebuild the tainted image.
```bash
cameron@host:~/GaiaPlatform/production$ echo "this taints the image!" > foo.txt
cameron@host:~/GaiaPlatform/production$ gdev build --tainted
# Ok. The image will be named `<name>:tainted` and will not use the git hash as the tag.
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
run.
```bash
cameron@host:~/GaiaPlatform/production$ gdev run --mixins gdb
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

## Best-practices
TODO

### Delete third-party git repositories at the end of the `[run]` section
TODO

### Avoid `--tainted` if you can easily rebuild from inside the container
TODO

## Multi-stage build and merge process
TODO
