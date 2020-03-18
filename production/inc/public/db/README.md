# production/inc/api/db
This folder contains the public interface to the database engine.

## storage engine
Currently, se.hpp is the engine that has been used for demonstration. Upgrade or replace this one when the next version is ready.

Programs that include se.hpp will need to link with libuuid.so. It is a C library able to generate 128-bit UUIDs. It's available to install:
```
sudo apt-get install uuid-dev
```
