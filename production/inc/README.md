# production/inc
This folder contains the public internal and external interfaces of our production components.
Internal interfaces should reside directly under the component name:
```
inc/db/my_internal_db_header.hpp
```

External interfaces should reside under **api** and then the component name:
```
inc/api/db/my_external_db_header.hpp
```
