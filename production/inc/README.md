# production/inc
This folder contains the public and internal interfaces of our production components.

Internal headers should be placed by component under _gaia_internal/_. For example:
```
inc/gaia_internal/db/db.hpp
```

Public headers should be placed by component under _gaia/_. Public headers are used by customers of the Gaia SDK.  Public headers must only include other public headers.  Like internal headers, public headers are also grouped by component.  Because the public headers are under a top-level _gaia/_ subdirectory, there is no need to prefix header filenames with "gaia_".  For example:
```
inc/gaia/db/db.hpp // not gaia_db.hpp
```

Public headers that are common to all components go directly under _gaia/_. For example:
```
inc/gaia/common.hpp // not inc/gaia/common/common.hpp
```

In order to simplify our headers, we expose fewer component groups under _gaia/_ than under _gaia\_internal/_. The current mapping is as follows (but definitely not set in stone):
* System and common headers go directly under _inc/gaia/_
* Catalog and database headers go under _inc/gaia/db/_
* Rules go under _inc/gaia/rules/_
* Direct access headers go under _inc/gaia/direct\_access/_
