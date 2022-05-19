This folder contains opensource projects that have been modified to meet gaia requirements. So far, the only modification is to rename the namespaces to avoid conflicts with different versions of the same library in user code.

Each project in this folder has a corresponding folder in `third_party/production`. The third party project builds and wrap the bundle project into a cmake library that can be used by other Gaia modules. Eg: `third_party/bundle/gaia_spdlog -> third_party/production/gaia_spdlog`

See `production/licenses/LICENSE.third-party.txt` for third party license information.
