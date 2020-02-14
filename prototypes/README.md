# prototypes
This is a folder for prototype code that is not meant for production but may be used for proof of concept demos. 

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

