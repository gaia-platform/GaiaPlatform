# docs
This folder enables doxygen builds locally as controlled by configuration settings in *doxyfile*.

_Note that documentation builds are *not* included as part of the top level production build.  You must explicitly build this directory._

## To Build

* Install doxygen ```sudo apt install doxygen```
* Create a subfolder **build\** and then execute the following commands in it:
* ```cmake ..```
* ```make```

## To View

The build will generate documentation based on doxygen comments in the code under a *docs* directory.  You can view them as either *html* or *latex* files.  The easiest way to look at them is to point your browser at ```index.html``` under the *html* directory and then use the links to navigate your content.

