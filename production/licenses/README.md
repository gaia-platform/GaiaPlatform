# licenses
This is a folder for collecting license information for third-party code that is used by the shipped production code. Each instance should be documented in this file.

The license notices that have to be distributed with our code are collected in [LICENSE.third-party.txt](https://github.com/gaia-platform/GaiaPlatform/blob/main/production/licenses/LICENSE.third-party.txt).

Hence, whenever a dependency on a new third-party component is added, you should do the following:
* Add an entry to the list in this file.
* If necessary, update [LICENSE.third-party.txt](https://github.com/gaia-platform/GaiaPlatform/blob/main/production/licenses/LICENSE.third-party.txt) to add a new license text.

# Third-party Components

This list should be maintained in alphabetical order, for easier lookup.

* 1024cores.net - data structures
  * Apache: https://www.apache.org/licenses/LICENSE-2.0.html
  * BSD: https://www.1024cores.net/home/code-license
* backward-cpp
  * MIT: https://github.com/bombela/backward-cpp/blob/master/LICENSE.txt
* cpptoml
  * MIT: https://github.com/skystrife/cpptoml/blob/master/LICENSE
* daemonize
  * BSD: https://github.com/bmc/daemonize/blob/master/LICENSE.md
* flatbuffers
  * Apache: https://github.com/google/flatbuffers/blob/master/LICENSE.txt
* fmt
  * MIT: https://github.com/fmtlib/fmt/blob/master/LICENSE.rst
* json (json.hpp)
  * MIT: https://github.com/nlohmann/json/blob/develop/LICENSE.MIT
* libexplain
  * LGPL3: https://salsa.debian.org/debian/libexplain/-/blob/master/LICENSE
* liburing
  * License notes: https://github.com/axboe/liburing/blob/master/README
  * MIT: https://github.com/axboe/liburing/blob/master/LICENSE
* LLVM
  * Apache: https://llvm.org/docs/DeveloperPolicy.html#new-llvm-project-license-framework
* RocksDB
  * Apache: https://github.com/facebook/rocksdb/blob/master/LICENSE.Apache
  * BSD (LevelDB): https://github.com/facebook/rocksdb/blob/master/LICENSE.leveldb
* scope_guard
  * Public Domain: https://github.com/ricab/scope_guard/blob/main/LICENSE
* spdlog
  * MIT: https://github.com/gabime/spdlog/blob/v1.x/LICENSE
* spdlog_setup
  * MIT: https://github.com/guangie88/spdlog_setup/blob/master/LICENSE
* tabulate
  * MIT: https://github.com/p-ranav/tabulate/blob/master/LICENSE
  * Custom (termcolor): https://github.com/p-ranav/tabulate/blob/master/LICENSE.termcolor
  * Boost (Optional-Lite): https://github.com/p-ranav/tabulate/blob/master/LICENSE.optional-lite
  * Boost (Variant-Lite): https://github.com/p-ranav/tabulate/blob/master/LICENSE.variant-lite
