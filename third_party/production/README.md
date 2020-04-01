# third_party/production
This is third-party code that is approved for production use.

## pybind

Last pulled on 02/20/2020.

This library enables C++ code to be called from python.

This is a headers only library, so no installation is needed.

## flatbuffers

Last pulled on 02/27/2020, hash c9a30c9c.

This is the google flatbuffers serialization library.

No install is needed because we user our cmake process to build flatbuffers when needed.

For installing the python libraries, go to python subfolder and execute ```sudo python setup.py install```.

Detailed steps for installation of python libraries:

```
cd GaiaPlatform/third_party/production/flatbuffers/python
sudo python setup.py install
```

## googletest - tags/release-1.10.0

This is the googletest library that integrates with ctest and is used for C++ testing.

No install is needed, because we will use our cmake process to build the harness when needed.

