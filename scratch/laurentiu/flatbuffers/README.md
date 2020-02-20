# Flatbuffers tests

This folder contains various tests of the flatbuffers serialization library.

## Build instructions

* Get the repository: ```git clone https://github.com/google/flatbuffers.git```, then build with regular cmake steps.
  * To install flatc compiler, execute the following command from the build folder: ```sudo make install```.
* Get the repository: ```git clone https://github.com/dvidelabs/flatcc```, then build with regular cmake steps.
  * flatcc does not come with an installation script, so you need to manually perform these steps:
    * copy flatcc from bin folder to /usr/local/bin
    * copy flatcc folder from include folder to /usr/local/include
    * copy content of lib folder to /usr/local/lib

## Content overview

Here is the list of tests:

* Tutorial.cpp - a sequence of operations based on the tutorial steps from the official flatbuffers documentation. This test demonstrates how to create flatbuffers objects and how they can be modified. The test executable needs to be run from its binplaced location, to be able to find its input file, *monster_data.bin*. Other files related to this test are:
  * *monster.fbs* contains the definition of the Monster class that is used in the tutorial. This was compiled with command ```flatc --cpp --gen-mutable --gen-object-api monster.fbs```, to generate the header file *monster_generated.h*. Re-run the command if the input file is changed.
  * *monster_data.json* contains the JSON definition of an instance of the Monster class. This was processed with command ```flatc -b monster.fbs monster_data.json```, to generate the binary file *monster_data.bin*. Re-run the command if the input file is changed.
* Tutorial.c - a demonstration of the C language flatbuffers library. This test uses the *monster.fbs* type. To build it, you will need to install the *flatccrt* library, which can be built after enlisting into the flatcc git repository: *https://github.com/dvidelabs/flatcc*.
* TestVersions.cpp - a test that demonstrates how 2 versions of generated interfaces interact with 2 versions of data schemas. This test also needs to be executed from its binplaced location.
* TestAdvanced.cpp - this is a test for more advanced (i.e. less mainstream) flatbuffers operations. It demonstrates how the content of a flatbuffer can be parsed manually, using vtables and vtable offsets.
* TestAccess.cpp - if Table inheritance is changed to be public, then this test can be used to experiment with some of the Table interfaces.

