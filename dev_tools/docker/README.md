# Create environment for Gaia demo and evaluation programs
This Dockerfile is loosely based on the gdev build of all of the prerequisites for the Gaia build.

It loads libraries, builds open-source packages, and sets up the environment for our current set of demonstration programs.

This docker image is not intended for Gaia software development, as gdev is set up for all of that. Rather,
it should be made available to any user who may want to use the Gaia SDK without spending time setting
up the environment.

To build the Docker image, make sure this Dockerfile is in the current directory, then run:
```
docker build -t demo:latest .
```

To run this local image:
```
docker run -it --rm demo:latest
```
