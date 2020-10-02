# Gaia Style

This package folder contains source code that showcases how Gaia `.clang-tidy` and `.clang-format` configurations work.

Whenever you modify `.clang-tidy` or `.clang-format` you should updates the files in this folder accordingly to "show"
what the new change is about.

* `clang_format.cpp|clang_format.hpp`: show the formatting rules. You can create methods that showcase a formatting rule.
   For instance, to show how indentation of short statements work, you could create a `short_statements()` method.
* `clang_tyidy.cpp|clang_tidy.hpp`: show the linter rules. These files contain code that triggers the linter. 
   For instance: 
   ```
    // Should have _t suffix.
    struct structure
    {
    };
  ```

