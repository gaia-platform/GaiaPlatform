# GaiaObj
Base class for Extended Data Classes.

# perftimer
A simple performance timer utility class.  Include [PerfTimer.h](PerfTimer.h) to use.

Sample Usage:

```
int64_t ns;
PerfTimer(ns, [&]() {
        gaia_se::begin_transaction();
            crawl_edges_raw(gaia_se_node::open(src->Gaia_id()), dst->Gaia_id(), c);
        gaia_se::commit_transaction();
});
printf("Searched %d routes in %.2f ms using raw access\n", c.num_routes, PerfTimer::ts_ms(ns));
```
