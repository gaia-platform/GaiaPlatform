This contents of this folder run a sanity check on the SDKs built from master.

The check is done by creating a clean ubuntu20 (or ubuntu18) image, installing the SDK, building the samples, and running them.  If any step fails in this process then Teamcity will flag the job as failed.

The SDK teamcity job does the following:
```
cd dev_tools/sdk/test
echo $USER
docker build -t gaia_ubuntu_20 -f Dockerfile_gaia_ubuntu_20 .
docker build --no-cache -t gaia_sdk_20 -f Dockerfile_gaia_sdk_20 .
```

As you can see above, the first step build a clean ubuntu20 image and then install clang and cmake on top of that. The second step then builds on the `gaia_ubuntu20` container.  It installs the last successfully built SDK from *master*, builds the samples (`hello` and `incubator`) and then exits.  You can look at the output in the build.log to troubleshoot SDK build failures.