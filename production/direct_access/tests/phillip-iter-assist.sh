#!/usr/bin/env sh

# This script needs to be run in a gdev-generated docker container.

set -e

make -j$(nproc)

(cd /build/production/db/storage_engine && make install)

/build/production/db/storage_engine/gaia_se_server &
/build/production/direct_access/test_iterators \
    || pkill gaia_se_server && echo "Stopped SE server."

pkill gaia_se_server
