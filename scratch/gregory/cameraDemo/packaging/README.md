# Brief
From anywhere, run the [run.sh](./run.sh) that's included in the same folder as this README.
Expect a build to take an hour or two. You may interrupt it at any time and not lose significant
progress. The script outputs the executable `CameraDemo-$(arch).AppImage` in your CWD. The camera
demo runs on any supported Linux distro with a kernel version >= 4.18 (Ubuntu 18.04).

# Dependencies
- docker
- git

# Build Process
The entire build process is shown in the Dockerfile in the same folder as this README.

We use docker to build against a specific Linux distro and output a single executable package. The
output executable includes our camera demo binary, all of its shared library dependencies, and data
files. For the sake of portability, we use a slightly older Linux distro (Ubuntu 18.04). This
ensures that our packaged libraries don't use newer kernel features that aren't present on older,
still-supported Linux distros.

# AppImage Details
Most of this information can be found in https://docs.appimage.org/packaging-guide/manual.html.

This build creates a standalone executable of the Gaia Platform camera demo that includes all the
shared library and data file dependencies it needs to run on any Linux system. More specifically,
the output executable self-mounts a fuse filesystem with the following structure.
```
<AppImage root>
    |- AppRun
    |- camera_demo.desktop
    |- camera_demo.png
    |- usr
        |- bin
            |- camera_demo
        |- lib
            |- <OpenCV shared library>
            |- ...
            |- <Any shared library that isn't known to be on all linux distros>
            |- ...
```
### `./AppRun`
After the CamereDemo.AppImage self-mounts, it executes the `<AppImage root>/AppRun` entrypoint. It
changes the process CWD to `<AppImage root>` and executes `./usr/bin/camera_demo`.

### `./camera_demo.{desktop|png}`
These are metadata and a desktop icon needed by `./AppRun` and the AppImage build tool to find our
actual camera demo executable and give it an icon.

### `./usr/...`
All of the binaries and shared libraries here are modified from the originals. The original strings
like `/usr/...` are changed to `./usr/...` so that libraries are loaded from within the AppImage
before falling back to external system libraries.
