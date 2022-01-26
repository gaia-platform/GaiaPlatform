# docs
This folder enables doxygen builds locally as controlled by configuration settings in *doxyfile*.

## Note

Documentation builds are *not* included as part of the top level production build currently for the following reasons:

* The cmake file currently uses add_custom_target such that it runs every time one builds.  Doxygen does not support incremental builds to my knowledge.  It walks over the entire code base for the directories specified in the *doxyfile* and rebuilds the documentation from scratch every time.
* Doxygen builds can take a long time - certainly longer than small changes in an incremental build.
* Currently doxygen spews a ton of warnings about various parts of the code not being documented.  I'm sure these warnings can be suppressed but I didn't want to pollute our build logs with this spew at ths time.

In other words, please do not include the _docs_ directory in the top level production cmake files at this time.

## To Build

* Install doxygen ```sudo apt install doxygen```
* Create a subfolder **build** and then execute the following commands in it:
* ```cmake ..```
* ```make```

## To View

The build will generate documentation based on doxygen comments in the code under a *docs* directory.  You can view them as either *html* or *latex* files.  An easy way to look at them is to point your browser at ```index.html``` under the *html* directory and then use the links to navigate your content.
