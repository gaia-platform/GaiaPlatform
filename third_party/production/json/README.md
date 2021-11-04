# JSON project
This directory contains a single header file, `json.hpp`.
It is part of a much larger project on github, `nlohmann/json`.
This is the only file that we currently need.

The header file is in this directory, but in its original project it is stored in a subdirectory named `json/single_include/nlohmann`.
To refer to this header in cmake, use `${JSON_INC}` in the project's `INCLUDES` list.

We may want to create a cmake or gdev file to handle all of this automatically.
However, this single header file contains all of the functionality required for JSON manipulation.
