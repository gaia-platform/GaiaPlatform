/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/////////////////////////////////////////////
// Portions of this code are derived
// from TinkerGraph project.
// Used under Apache License 2.0
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.io.File;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;

import org.apache.commons.configuration.BaseConfiguration;
import org.apache.commons.configuration.Configuration;

import org.apache.tinkerpop.gremlin.process.traversal.dsl.graph.GraphTraversalSource;
import org.apache.tinkerpop.gremlin.structure.T;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;

import static org.apache.tinkerpop.gremlin.structure.io.IoCore.gryo;

public final class TrueGraphDBFactory {

    private TrueGraphDBFactory()
    {
    }

    private static TrueGraphDBGraph getTrueGraphDBGraphWithNumberManager()
    {
        final Configuration configuration = getNumberIdManagerConfiguration();
        return TrueGraphDBGraph.open(configuration);
    }

    private static Configuration getNumberIdManagerConfiguration()
    {
        final Configuration configuration = new BaseConfiguration();

        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_VERTEX_ID_MANAGER,
            TrueGraphDBGraph.DefaultIdManager.INTEGER.name());
        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_EDGE_ID_MANAGER,
            TrueGraphDBGraph.DefaultIdManager.INTEGER.name());
        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_VERTEX_PROPERTY_ID_MANAGER,
            TrueGraphDBGraph.DefaultIdManager.LONG.name());
        
        return configuration;
    }

    // Classic graph data set.
    public static TrueGraphDBGraph createClassic()
    {
        final Configuration configuration = new BaseConfiguration();
   
        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_VERTEX_ID_MANAGER,
            TrueGraphDBGraph.DefaultIdManager.INTEGER.name());
        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_EDGE_ID_MANAGER,
            TrueGraphDBGraph.DefaultIdManager.INTEGER.name());
        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_VERTEX_PROPERTY_ID_MANAGER,
            TrueGraphDBGraph.DefaultIdManager.INTEGER.name());
   
        final TrueGraphDBGraph graph = TrueGraphDBGraph.open(configuration);

        generateClassic(graph);
   
        return graph;
    }

    public static void generateClassic(final TrueGraphDBGraph graph)
    {
        final Vertex marko = graph.addVertex(T.id, 1, "name", "marko", "age", 29);
        final Vertex vadas = graph.addVertex(T.id, 2, "name", "vadas", "age", 27);
        final Vertex lop = graph.addVertex(T.id, 3, "name", "lop", "lang", "java");
        final Vertex josh = graph.addVertex(T.id, 4, "name", "josh", "age", 32);
        final Vertex ripple = graph.addVertex(T.id, 5, "name", "ripple", "lang", "java");
        final Vertex peter = graph.addVertex(T.id, 6, "name", "peter", "age", 35);

        marko.addEdge("knows", vadas, T.id, 7, "weight", 0.5f);
        marko.addEdge("knows", josh, T.id, 8, "weight", 1.0f);
        marko.addEdge("created", lop, T.id, 9, "weight", 0.4f);
        josh.addEdge("created", ripple, T.id, 10, "weight", 1.0f);
        josh.addEdge("created", lop, T.id, 11, "weight", 0.4f);
        peter.addEdge("created", lop, T.id, 12, "weight", 0.2f);
    }

    // Modern graph data set.
    public static TrueGraphDBGraph createModern()
    {
        final TrueGraphDBGraph graph = getTrueGraphDBGraphWithNumberManager();
        generateModern(graph);
        return graph;
    }

    public static void generateModern(final TrueGraphDBGraph graph)
    {
        final Vertex marko = graph.addVertex(T.id, 1, T.label, "person", "name", "marko", "age", 29);
        final Vertex vadas = graph.addVertex(T.id, 2, T.label, "person", "name", "vadas", "age", 27);
        final Vertex lop = graph.addVertex(T.id, 3, T.label, "software", "name", "lop", "lang", "java");
        final Vertex josh = graph.addVertex(T.id, 4, T.label, "person", "name", "josh", "age", 32);
        final Vertex ripple = graph.addVertex(T.id, 5, T.label, "software", "name", "ripple", "lang", "java");
        final Vertex peter = graph.addVertex(T.id, 6, T.label, "person", "name", "peter", "age", 35);

        marko.addEdge("knows", vadas, T.id, 7, "weight", 0.5d);
        marko.addEdge("knows", josh, T.id, 8, "weight", 1.0d);
        marko.addEdge("created", lop, T.id, 9, "weight", 0.4d);
        josh.addEdge("created", ripple, T.id, 10, "weight", 1.0d);
        josh.addEdge("created", lop, T.id, 11, "weight", 0.4d);
        peter.addEdge("created", lop, T.id, 12, "weight", 0.2d);
    }

    // The Crew data set.
    public static TrueGraphDBGraph createTheCrew()
    {
        final Configuration configuration = getNumberIdManagerConfiguration();

        configuration.setProperty(
            TrueGraphDBGraph.TRUEGRAPHDB_DEFAULT_VERTEX_PROPERTY_CARDINALITY,
            VertexProperty.Cardinality.list.name());

        final TrueGraphDBGraph graph = TrueGraphDBGraph.open(configuration);

        generateTheCrew(graph);

        return graph;
    }

    public static void generateTheCrew(final TrueGraphDBGraph graph)
    {
        final Vertex marko = graph.addVertex(T.id, 1, T.label, "person", "name", "marko");
        final Vertex stephen = graph.addVertex(T.id, 7, T.label, "person", "name", "stephen");
        final Vertex matthias = graph.addVertex(T.id, 8, T.label, "person", "name", "matthias");
        final Vertex daniel = graph.addVertex(T.id, 9, T.label, "person", "name", "daniel");
        final Vertex gremlin = graph.addVertex(T.id, 10, T.label, "software", "name", "gremlin");
        final Vertex tinkergraph = graph.addVertex(T.id, 11, T.label, "software", "name", "tinkergraph");

        marko.property(VertexProperty.Cardinality.list, "location", "san diego", "startTime", 1997, "endTime", 2001);
        marko.property(VertexProperty.Cardinality.list, "location", "santa cruz", "startTime", 2001, "endTime", 2004);
        marko.property(VertexProperty.Cardinality.list, "location", "brussels", "startTime", 2004, "endTime", 2005);
        marko.property(VertexProperty.Cardinality.list, "location", "santa fe", "startTime", 2005);

        stephen.property(VertexProperty.Cardinality.list, "location", "centreville", "startTime", 1990, "endTime", 2000);
        stephen.property(VertexProperty.Cardinality.list, "location", "dulles", "startTime", 2000, "endTime", 2006);
        stephen.property(VertexProperty.Cardinality.list, "location", "purcellville", "startTime", 2006);

        matthias.property(VertexProperty.Cardinality.list, "location", "bremen", "startTime", 2004, "endTime", 2007);
        matthias.property(VertexProperty.Cardinality.list, "location", "baltimore", "startTime", 2007, "endTime", 2011);
        matthias.property(VertexProperty.Cardinality.list, "location", "oakland", "startTime", 2011, "endTime", 2014);
        matthias.property(VertexProperty.Cardinality.list, "location", "seattle", "startTime", 2014);

        daniel.property(VertexProperty.Cardinality.list, "location", "spremberg", "startTime", 1982, "endTime", 2005);
        daniel.property(VertexProperty.Cardinality.list, "location", "kaiserslautern", "startTime", 2005, "endTime", 2009);
        daniel.property(VertexProperty.Cardinality.list, "location", "aachen", "startTime", 2009);

        marko.addEdge("develops", gremlin, T.id, 13, "since", 2009);
        marko.addEdge("develops", tinkergraph, T.id, 14, "since", 2010);
        marko.addEdge("uses", gremlin, T.id, 15, "skill", 4);
        marko.addEdge("uses", tinkergraph, T.id, 16, "skill", 5);

        stephen.addEdge("develops", gremlin, T.id, 17, "since", 2010);
        stephen.addEdge("develops", tinkergraph, T.id, 18, "since", 2011);
        stephen.addEdge("uses", gremlin, T.id, 19, "skill", 5);
        stephen.addEdge("uses", tinkergraph, T.id, 20, "skill", 4);

        matthias.addEdge("develops", gremlin, T.id, 21, "since", 2012);
        matthias.addEdge("uses", gremlin, T.id, 22, "skill", 3);
        matthias.addEdge("uses", tinkergraph, T.id, 23, "skill", 3);

        daniel.addEdge("uses", gremlin, T.id, 24, "skill", 5);
        daniel.addEdge("uses", tinkergraph, T.id, 25, "skill", 3);

        gremlin.addEdge("traverses", tinkergraph, T.id, 26);

        graph.variables().set("creator", "marko");
        graph.variables().set("lastModified", 2014);
        graph.variables().set("comment", "this graph was created to provide examples and test coverage for tinkerpop3 api advances");
    }

    // Kitchen Sink data set.
    public static TrueGraphDBGraph createKitchenSink()
    {
        final TrueGraphDBGraph graph = getTrueGraphDBGraphWithNumberManager();
        generateKitchenSink(graph);
        return graph;
    }

    public static void generateKitchenSink(final TrueGraphDBGraph graph)
    {
        final GraphTraversalSource g = graph.traversal();

        g.addV("loops").property(T.id, 1000).property("name", "loop").as("me")
            .addE("self").to("me").property(T.id, 1001)
            .iterate();

        g.addV("message").property(T.id, 2000).property("name", "a").as("a")
            .addV("message").property(T.id, 2001).property("name", "b").as("b")
            .addE("link").from("a").to("b").property(T.id, 2002)
            .addE("link").from("a").to("a").property(T.id, 2003).iterate();
    }

    // COW data set.
    public static TrueGraphDBGraph createCOW()
    {
        final TrueGraphDBGraph graph = getTrueGraphDBGraphWithNumberManager();
        generateCOW(graph);
        return graph;
    }

    public static void generateCOW(final TrueGraphDBGraph graph)
    {
        final Vertex node1 = graph.addVertex(T.id, 1, T.label, "1", "payload", "n1");
        final Vertex node2 = graph.addVertex(T.id, 2, T.label, "1", "payload", "n2");
        final Vertex node3 = graph.addVertex(T.id, 3, T.label, "2", "payload", "n3");
        final Vertex node4 = graph.addVertex(T.id, 4, T.label, "2", "payload", "n4");

        node1.addEdge("3", node2, T.id, 5, "payload", "e5=n1->n2");
        node1.addEdge("3", node3, T.id, 6, "payload", "e6=n1->n3");
        node4.addEdge("4", node1, T.id, 7, "payload", "e7=n4->n1");
        node2.addEdge("4", node3, T.id, 8, "payload", "e8=n2->n3");
    }
}
