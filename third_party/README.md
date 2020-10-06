# third_party
This is third-party code that is approved for internal use for either production or evaluation.

## Add dependencies
1. Add the dependency and keep the information updated in
[the open source software tracking spreadsheets](https://docs.google.com/spreadsheets/d/1xmSwPZsEAb50zUDuABF3JKRbJk_tNeBsVAndtTmKBLM/edit#gid=0).

2. Create the corresponding gdev entry for the build target and add the gdev
configuration file for the dependency. For more details, check out
[the gdev doc](/dev_tools/gdev/README.md). Example configurations can be found in
individual directories for all depdendencies except legacy source depdendencies.

3. (Optional) for light weight C++ dependencies, consider adding a `CMakeLists.txt` for local
build. Examples can be found in the following dependencies:
- [backward](production/backward/CMakeLists.txt)
- [cpptoml](production/cpptoml/CMakeLists.txt)
- [fmt](production/fmt/CMakeLists.txt)
- [spdlog](production/spdlog/CMakeLists.txt)
- [spdlog_setup](production/spdlog_setup/CMakeLists.txt)
- [tabulate](production/tabulate/CMakeLists.txt)


