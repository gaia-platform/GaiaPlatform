This folder contains opensource projects that have been modified to meet gaia requirements. So far, the only modification performed has been to rename the namespace to avoid conflicts with different versions of the same library in user code.

Each project in this folder has a corresponding folder in `third_party/production`, that builds it and wrap into a cmake library that can be used from the Gaia modules. Eg: `third_party/bundle/gaia_spdlog -> third_party/production/gaia_spdlog`

TODO we need to give proper credit to the used open source software, I created a follow-up task to do so: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-990
