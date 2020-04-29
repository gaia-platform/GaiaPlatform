# airport-related work

## Folder descriptions

* **graphml** contains a tool for converting the raw airport CSV data into graphml format. It expects the original data files to be in the folder in which it is executed. Calling it with an argument will cause it to generate detailed debug output.
* **flatbuffers** contains a flatbuffers schema for the vertex payloads of an airport graph structured in the same way as the graphml data.
