# scratch
This is a scratch folder for non-production code. Experiments go here. 

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

