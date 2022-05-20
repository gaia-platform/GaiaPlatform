# C++ Coding Guidelines
There may be exceptions justifying going against guidelines.
However, they should be well-justified exceptions, not habits.

## Comments
1. Follow grammatical rules when writing comments. Capitalize the starting word in a sentence, terminate sentences with dots, pay attention to typos and misspellings.
2. Avoid comments added at the end of lines in favor of comments inserted above the code that they are referring to.
3. Avoid C-style comments (`/* */`) in favor of C++ comments (`//`).

## Naming
1. We will use **snake_casing** for all variable and function names. All letters will be lowercase with words separated by underscores. For macros we will use uppercase **SNAKE_CASING**. Types will have the `_t` suffix added to their names. For example: `class memory_record_t`.
2. We do not use Hungarian notation.
3. Use constants instead of literal values. This also helps you document the purpose of the constant.
4. Consider assigning parts of complex expressions to intermediate values, whose names can also help explain the operation.
5. Identifiers used for numerical quantities should include in their names the measurement unit corresponding to their value.

### File extensions
C++ code files should use a `.cpp` extension.

C++ header files should use an `.hpp` extension.

To better separate declarations from definitions, inline method definitions and template definitions should be placed in an `.inc` file that should be included at the end of the header file using, after all declarations.

`inc` files **should not**:
1. use `pragma once`.
2. include any files of their own.
    1. any required headers should be included by the parent header file.
3. declare any namespaces.

`inc` files **should always**:
1. be included in a single header file.
2. be included using `include "XYZ.inc"` syntax instead of `include <XYZ.inc>` syntax.

#### Include guards
For include files we will use `#pragma once` rather than macro-based "include guards".

### Namespaces
All code should be under a `gaia` namespace with major components having their own sub-namespace.

***Example***: `gaia.db.memory_manager`

### Variable Name Prefixes
The following prefixes should be used to indicate the type of variables:

| Prefix | Description |
| --- |  --- |
| `c_` | Constant |
| `g_` | Global |
| `s_` | Static |
| `m_` | Class member |
| `e_` | Enum value (see use note below) |

**Note:** these prefixes do not combine. Their order also indicates the order of preference in which they should be applied. I.e. a global constant should be prefixed with `c_`, not `g_`.

**Note on the use of the `e_` prefix:** Normally, enum values do not require any special prefix, because we recommend their definition as “enum class”, which leads to references of the form `enum_name::enum_value_name`. However, if your enum value names conflict with reserved words, we recommend adding the `e_` prefix to all value names of that enum, to prevent such conflicts.

### Variable Name Suffixes
The following suffixes are optional but can be used to provide extra clarity:

| Suffix | Description |
| --- |  --- |
| `_ptr` | Pointer type |
| `_fn` | Function type |

## Structure

### Struct declarations
Only use structs for public entities. Ordering should be:
1. Fields
2. Methods

### Class declarations
For classes, the following ordering rules have to be applied and their order also indicates their priority:
1. Public before private. All private declarations must follow public ones.
2. Static before non-static. All static declarations must precede non-static ones.
3. Methods before fields. All method declarations must precede field declarations.

Furthermore, the public/private specifiers should be repeated at the beginning of each new category of declarations, to help as delimiters of these sections.

**Example:**
```
class some_class_t
{
public:
    // static methods.

public:
    // static fields.

public:
    // non-static methods.

public:
    // non-static fields.

private:
    // static methods.

private:
    // static fields.

private:
    // non-static methods.

private:
    // non-static fields.
};
```

### Implementation
Within a `.cpp` file, always place called methods before the methods calling them.
This enables easier code reviewing because the implementation can be reviewed bottom-up (structurally - top-down as you scroll through the page) in a linear fashion.

### Header files
1. For a given header file `XYZ.cpp`, declare its interface in `XYZ.hpp`.
2. The `XYZ.hpp` file should only include the headers needed for the interface declarations, so that its users don’t have to take unnecessary dependencies.
3. Include headers in the following order:
    1. associated file header (XYZ.hpp)
    2. C system headers
    3. C++ standard library headers
    4. third party library headers
    5. your other project headers.
4. Do not use “using namespace” declarations in header files. Only use them in cpp files, if at all.

### Indentation and formatting
1. Try to leverage the default VSCode indentation. Formatting an existing block of code can be achieved by selecting it and using the Ctrl-K Ctrl-F shortcut.
2. For C++ code, tabs should be replaced with 4 spaces. Configure your editor to do this automatically.
3. There should be no empty space at the end of lines.
4. Code files should always end with an empty line.

#### Block braces
Block opening braces should be placed on a separate line.

**Example:**
```
// Bad:
if (condition) {
}
// Good:
if (condition)
{
}
```

#### Operator spacing
Use spaces around operators and keywords.

**Example:**
```
// Bad:
if(condition)
{
    result=argument_one*argument_two;
}
// Good:
if (condition)
{
    result = argument_one * argument_two;
}
```

#### Type operators
When using `*` and `&` to designate pointers and references, keep them attached to the types rather than to the variables.

**Example:**
```
// Bad:
int *some_function(
    some_class &some_argument,
    int * some_other_argument)
// Good:
int* some_function(
    some_class& some_argument
    int* some_other_argument)
```

#### Line breaking
1. Avoid long lines, but if they cannot be avoided, then break them before operators, so that the continuation lines start with an operator.

**Example:**
```
// Good:
int complex_result = (parameter_one + parameter_two)
    / (parameter_three - parameter_four);
```

2. When breaking the arguments of a call over multiple lines, start with the first argument so that the continuation lines use the default 1-TAB indentation.

**Example:**
```
// Good:
bool result = function_call(
    argument_one,
    argument_two);
```

3. Avoid formatting that can be easily disturbed by name changes or additions of expressions.

**Example:**
```
// Bad:
bool result = function_call(argument_one,
                            argument_two);
```

### Folder structure
The following folder structure will be used for C++ code:

* `/inc/gaia` (public facing interfaces)
  * `/component_name`
* `/inc/gaia_internal` (internal facing interfaces)
  * `/component_name`
* `/component_name`
  * `/inc` (interfaces that are external for subcomponents, but not for component)
    * `/sub_component_name`
  * `/sub_component_name`
    * `/inc` (internal interfaces of subcomponent)
    * `/src`
    * `/tests`
  * `/tests`
* `/tests`

The top-level inc folder will contain global public and internal interfaces.
Our build system should be set up to include only this top folder in its search path, so that any header file under it will have to be included with the folder structure leading to it.
For example:
```
#include “gaia/db/events.hpp”
#include “gaia_internal/common/retail_assert.hpp”
```

Some additional notes on this scheme:
* We should only keep one folder level under the `gaia` and `gaia_internal` folders. The names of the folders should correspond to namespace names.
* For the public `gaia` folder, all `gaia::common namespace` headers will be collected directly in the root, rather than under a dedicated `gaia/common` folder.
* Header files should contain declarations within a single namespace. Some accepted exceptions are listed below.
  * An obvious exception to this rule are the header files that contain exception declarations. These are collected in single files, to make them easier to look up. Thus, we have a `gaia/exceptions.hpp` header file for all database engine exceptions and a `gaia/rules/exceptions.hpp` header file for all rules engine exceptions (which also includes the database exceptions file). The internal implementations of exceptions are collected under the corresponding files: `gaia_internal/exceptions.hpp` and `gaia_internal/rules/exceptions.hpp`, respectively.
  * Another exception to this rule is when headers need to make additions to the std namespace, such as is the case for implementations of `std::hash()`.

## Language feature restrictions
Avoid using `#define` macros when possible.

Avoid using `auto` when the type cannot be easily inferred.

Use explicit integer size types: `int32_t` instead of `long`.

## Common abbreviations and acronyms
This list is provided to avoid getting multiple types of abbreviations or acronyms for common terms. The recommendation is to either use the full term or the abbreviation or acronym listed in this table.

| Term | Abbreviation or acronym |
| --- |  --- |
| Database | `db` |
| File descriptor | `fd` |
| Iterator | `it` |
| Transaction | `txn` |

## Code Review Checklist

### For submitters
* Perform a code review yourself. Look out for typos or code that goes against these guidelines.
* Do not combine unrelated changes; separate them instead.
* Do not include unused code unless you know that you will follow up with another change that will use it.
* Do not postpone any opportunities for small improvements.
* If the code can be unit tested, include unit tests; if not, prepare a good argument.
* Ensure that your change does not impact regular build. Check that all necessary files are part of your change by doing a clean build.

### For reviewers
* Check for code and design clarity and opportunities to make them clearer.
* Check for all the items in the submitter’s checklist.
