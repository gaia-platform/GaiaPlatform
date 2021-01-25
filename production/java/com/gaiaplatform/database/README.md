# java/com/gaiaplatform/database
This is a folder for GaiaPlatform database-related Java code.

## Tinkerpop CacheGraph provider

Implements a Tinkerpop provider that allowed the original COW database prototype to be modified through Gremlin tools.

Currently, this code is no longer functional and needs a major rewrite to get it to work again with the new Gaia database engine.

The provider is written in Java and packaged as a JAR file - *GaiaTinkerpop.jar*. To build it, you need the following:

1. **Java** - Install package *openjdk-8-jdk*. Version 8 is recommended to avoid running into several warnings due to deprecated features.

2. **Gremlin console and server** - These tools are distributed as zip files and can be downloaded from http://tinkerpop.apache.org/. They should be unzipped under **/usr/local/share/** and the resulting folder names should be renamed to **gremlin-console** and **gremlin-server**. The cmake files expect to find the console and server binaries in these locations.

3. **GaiaTinkerpop.jar** - after the previous 2 steps are performed, the GaiaTinkerpop JAR can be produced by building the **production/** folder. You will find the JAR file under the **build/** folder.

4. **Setting up the Tinkerpop provider** - To use the Tinkerpop provider with the Gremlin console and server, you need to create the following folder paths under both console and server folders: **ext/gaia-tinkerpop/lib/** and **ext/gaia-tinkerpop/plugin/**. Then copy the JAR file to all of these locations (to all 4 of them). You will also need to enable Java to find the native library that wraps the database by setting the LD_LIBRARY_PATH environment variable to point to its location using a command like: ```export LD_LIBRARY_PATH=<your_path_here>/GaiaPlatform/production/build/db/core```.
  * For the client, you can use the command ```import com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheGraph``` and then execute ```graph = CacheGraph.open()```.
  * For the server, you can execute ```graph = com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheGraph.open()``` (no import command appears to be available for this scenario).
  * You can also similarly use the CacheFactory class to initialize a graph with the structure used in the Python demo: ```graph = CacheFactory.createCowSample()```.
  * Once you create the graph, use the following command to start querying the graph: ```g = graph.traversal()```. For example, enter ```g.E().elementMap()``` to inspect the edges of the graph. Use the command ```:exit``` to exit the Gremlin console.
