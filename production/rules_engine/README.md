# rules_engine
This is a folder for the rules engine code.

The following folder structure is recommended for C++ projects:

* inc (public facing interfaces)
  * component_name
* component_name
  * inc (interfaces that are external for subcomponents, but not for component)
    * sub\_component\_name
  * sub\_component\_name
    * inc (internal interfaces of subcomponent)
    * src
    * tests
  * tests

