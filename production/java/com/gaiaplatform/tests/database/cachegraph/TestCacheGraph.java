/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.tests.database.cachegraph;

import java.io.IOException;
import java.util.List;

import org.apache.tinkerpop.gremlin.process.traversal.dsl.graph.__;
import org.apache.tinkerpop.gremlin.process.traversal.dsl.graph.GraphTraversalSource;

import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.T;
import org.apache.tinkerpop.gremlin.structure.Vertex;

import static org.apache.tinkerpop.gremlin.structure.io.IoCore.graphml;

import com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheFactory;
import com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure.CacheGraph;

public final class TestCacheGraph
{
    private static final String BASIC = "basic";
    private static final String FACTORY = "factory";
    private static final String GRAPHML = "graphml";

    public static void main(String[] args)
    {
        if (args[0].equals(BASIC))
        {
            testBasicGraphOperations();
        }
        else if (args[0].equals(FACTORY))
        {
            testFactoryOperations();
        }
        else if (args[0].equals(GRAPHML))
        {
            testGraphmlLoad();
        }
        else
        {
            System.out.println("Invalid TestCacheGraph argument");
            System.exit(1);
        }

        System.exit(0);
    }

    protected static void checkGraphSize(GraphTraversalSource g, final long countVertices, final long countEdges)
    {
        assert g.V().count().next() == countVertices;
        assert g.E().count().next() == countEdges;
    }

    public static void testBasicGraphOperations()
    {
        CacheGraph graph = CacheGraph.open();

        CacheFactory.enableDebuggingMessages(graph, true);

        final Vertex alice = graph.addVertex(T.id, 1, T.label, "person", "name", "Alice");
        final Vertex bob = graph.addVertex(T.id, 2, T.label, "person", "name", "Bob");
        final Vertex charles = graph.addVertex(T.id, 3, T.label, "person", "name", "Charles");
        final Vertex fido = graph.addVertex(T.id, 4, T.label, "pet", "name", "Fido");

        alice.addEdge("likes", bob, T.id, 11, "how_long", "2");
        alice.addEdge("dislikes", charles, T.id, 12, "how_long", "6");
        alice.addEdge("owns", fido, T.id, 13, "how_long", "3");

        GraphTraversalSource g = graph.traversal();

        checkGraphSize(g, 4, 3);

        assert g.V(1).values("name").next().equals("Alice");
        assert g.V(2).values("name").next().equals("Bob");
        assert g.V(3).values("name").next().equals("Charles");
        assert g.V(4).values("name").next().equals("Fido");

        assert g.E(11).label().next().equals("likes");
        assert g.E(12).label().next().equals("dislikes");
        assert g.E(13).label().next().equals("owns");

        g.E(13).next().remove();

        checkGraphSize(g, 4, 2);

        assert g.V(4).both().count().next() == 0;

        g.V(4).next().remove();

        checkGraphSize(g, 3, 2);

        g.V(1).next().remove();

        checkGraphSize(g, 2, 0);
    }

    public static void testFactoryOperations()
    {
        // Create all the Tinkerpop sample graphs.
        CacheGraph graph = CacheFactory.createClassic();
        GraphTraversalSource g = graph.traversal();
        checkGraphSize(g, 6, 6);

        graph = CacheFactory.createModern();
        g = graph.traversal();
        checkGraphSize(g, 6, 6);

        graph = CacheFactory.createTheCrew();
        g = graph.traversal();
        checkGraphSize(g, 6, 14);

        graph = CacheFactory.createKitchenSink();
        g = graph.traversal();
        checkGraphSize(g, 3, 3);

        // Create all Gaia sample graphs.
        graph = CacheFactory.createCowSample();
        g = graph.traversal();
        checkGraphSize(g, 4, 4);

        graph = CacheFactory.createTinyAirport();
        g = graph.traversal();
        checkGraphSize(g, 39, 87);

        graph = CacheFactory.createTinyQ1Airport();
        g = graph.traversal();
        checkGraphSize(g, 9, 32);
    }

    public static void testGraphmlLoad()
    {
        CacheGraph graph = CacheGraph.open();

        try
        {
            graph.tx().open();
            graph.io(graphml()).readGraph("../../data/internal/airport/gaia-airport-tiny.graphml");
            graph.tx().commit();
        }
        catch (Exception e)
        {
            System.out.println(
                "An error happened while attempting to load graphml file: "
                + e.getMessage());
            System.exit(1);
        }

        GraphTraversalSource g = graph.traversal();

        checkGraphSize(g, 44, 102);

        // Check absence and presence of some airports.
        assert g.V().has("iata", "AUS").hasNext() == false;
        assert g.V().has("iata", "OTP").hasNext() == true;

        // Which airports are directly reachable from SEA?
        // There should be 3: AMS, CDG, and JFK.
        List<Object> airports = g.V().has("iata", "SEA").out().out("arrives_at").dedup().values("iata").fold().next();
        assert airports.size() == 3;
        assert airports.get(0).equals("AMS") || airports.get(0).equals("CDG") || airports.get(0).equals("JFK");
        assert airports.get(1).equals("AMS") || airports.get(1).equals("CDG") || airports.get(1).equals("JFK");
        assert airports.get(2).equals("AMS") || airports.get(2).equals("CDG") || airports.get(2).equals("JFK");
    }
}
