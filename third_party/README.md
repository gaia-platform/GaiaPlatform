# third_party
This is third-party code that is approved for internal use for either production or evaluation.

## Add dependencies
Add and keep updating the latest dependency information in
[the open source software tracking spreadsheets](https://docs.google.com/spreadsheets/d/1xmSwPZsEAb50zUDuABF3JKRbJk_tNeBsVAndtTmKBLM/edit#gid=0).

Create the corresponding gdev entry for the build target and add gdev configuration
file for the dependency. For more details, check out [the gdev doc](/gaia-platform/GaiaPlatform/blob/master/dev_tools/gdev/README.md).

(Optional) for light weight C++ dependencies, consider adding a `CMakeLists.txt` for local
build. Examples can be found in the following dependencies:
- [backward](production/backward/CMakeLists.txt)
- [cpptoml](production/cpptoml/CMakeLists.txt)
- [fmt](production/fmt/CMakeLists.txt)
- [spdlog](production/spdlog/CMakeLists.txt)
- [spdlog_setup](production/spdlog_setup/CMakeLists.txt)
- [tabulate](production/tabulate/CMakeLists.txt)


