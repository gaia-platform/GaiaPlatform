# SDK Sanity Test
This contents of this folder run a sanity check on the SDK debian package built from the *main* branch.

The check is done by creating a clean `Ubuntu 20.04` image, installing the SDK, building the samples, and running them.  If any step fails in this process then Teamcity will flag the job as failed.

A TeamCity `SDK` job triggers on successful build of **ProductionGaiaRelase_gdev**.

The SDK jobs have an artifact dependency on the `gaia-0.2.1_amd64.deb` file produced by the appropriate build.  This artifact is then copied into the docker image for testing.

The SDK TeamCity job does the following:
```
cd dev_tools/sdk/test
echo $USER
docker build -t gaia_ubuntu_20 -f Dockerfile_gaia_ubuntu_20 .
docker build --no-cache -t gaia_sdk_20 -f Dockerfile_gaia_sdk_20 .
```
In the above job, two docker images are created:

1. We are building a base `Ubuntu 20.04` image and then installing base requirements no top of that. See [Dockerfile_gaia_ubuntu_20](https://github.com/gaia-platform/GaiaPlatform/blob/main/dev_tools/sdk/test/Dockerfile_gaia_ubuntu_20).
1. We are then installing our SDK on top of this image, building the samples, and executing them. See [Dockerfile_gaia_sdk_20](https://github.com/gaia-platform/GaiaPlatform/blob/main/dev_tools/sdk/test/Dockerfile_gaia_sdk_20).
