# Incubator Demo
A demo of a rule-based control system for controlling the temperature of several incubators.

## Build Instructions
1. Build Gaia Platform production code.
2. Run cmake and suppliment the production build directory with the variable `GAIA_PROD_BUILD`.
```
> mkdir build && cd build
> cmake .. -DCMAKE_BUILD_TYPE=Debug -DGAIA_PROD_BUILD=../../../production/build 
> make
```

