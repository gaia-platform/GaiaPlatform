# third_party/production
This is third-party code that is approved for production use.

* pybind11 - Last pulled on 02/20/2020.
  * This is a headers only library, so no installation is needed.
* flatbuffers - Last pulled on 02/27/2020, hash c9a30c9c.
  * For installing C++ headers and library, do a regular cmake and then execute ```sudo make install``` from the build folder.
  * For installing the python libraries, go to python subfolder and execute ```sudo python setup.py install```.
* googletest - tags/release-1.10.0
  * No install needed as we will use our cmake process to build the harness when needed.

