# Direct Access Value-Linked Relationships Demo
TODO

## Build in `gdev`
TODO: remove this section after automating the build procedure.

```bash
cd /build/production
make install
cd /build/production/sdk
make install

mkdir -p /build/production/examples/direct_access_vlr
cd /build/production/examples/direct_access_vlr

gaia_db_server --persistence disabled & sleep 0.1
cmake /source/production/examples/direct_access_vlr
make -j$(nproc)
```
