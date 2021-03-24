# SDK Sanity Test
This contents of this folder run a sanity check on the SDK debian package built from the *master* branch.

The check is done by creating a clean `Ubuntu 20.04` (or `Ubuntu 18.04`) image, installing the SDK, building the samples, and running them.  If any step fails in this process then Teamcity will flag the job as failed.

Two TeamCity SDK jobs have been created:
* `SDK` - triggers on successful build of **ProductionGaiaRelase_gdev**. 
* `SDK_18_04` - triggers on successful build of **Production_gdev_18_04**.

The SDK jobs have an artifact dependency on the `gaia-0.1.0_amd64.deb` file produced by the appropriate build.  This artifact is then copied into the docker image for testing.

The SDK TeamCity job does the following:
```
cd dev_tools/sdk/test
echo $USER
docker build -t gaia_ubuntu_20 -f Dockerfile_gaia_ubuntu_20 .
docker build --no-cache -t gaia_sdk_20 -f Dockerfile_gaia_sdk_20 .
```
In the above job, two docker images are created:

1. We are building a base `Ubuntu 20.04` image and then installing base requirements no top of that. See [Dockerfile_gaia_ubuntu_20](https://github.com/gaia-platform/GaiaPlatform/blob/master/dev_tools/sdk/test/Dockerfile_gaia_ubuntu_20).
1. We are then installing our SDK on top of this image, building the samples, and executing them. See [Dockerfile_gaia_sdk_20](https://github.com/gaia-platform/GaiaPlatform/blob/master/dev_tools/sdk/test/Dockerfile_gaia_sdk_20).

The `SDK_18_04` on TeamCity works exactly the same as the `SDK` job except that everything is built on top of `Ubuntu 18.04`.
