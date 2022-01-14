# Value-Linked Relationships Demo
TODO

## Build in `gdev`
TODO: remove this section after automating the build procedure.

```bash
cd /build/production
make install
cd /build/production/sdk
make install

mkdir -p /build/production/examples/value_linked_relationships
cd /build/production/examples/value_linked_relationships

gaia_db_server --persistence disabled & sleep 0.1
cmake /source/production/examples/value_linked_relationships
make -j$(nproc)
```
