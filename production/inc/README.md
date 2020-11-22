# production/inc
This folder contains the public and internal interfaces of our production components.
```
inc/gaia_internal/<component_name> // internal headers
```

External interfaces should reside under **gaia** and then the component name:
```
inc/gaia/<component_name> // Public headers.  Common headers go under inc/gaia and not in a component directory.
```
