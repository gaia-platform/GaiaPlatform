////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

/////////////////////////////////////////////
// Portions of this code are derived
// from TinkerGraph project.
// Used under Apache License 2.0
/////////////////////////////////////////////

package com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure;

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

import static org.apache.tinkerpop.gremlin.structure.io.IoCore.graphml;

public final class CacheFactory
{
    private final static String LABEL_AIRPORT = "airport";
    private final static String LABEL_AIRLINE = "airline";
    private final static String LABEL_FLIGHT = "flight";
    private final static String LABEL_DEPARTURE = "departure";
    private final static String LABEL_ARRIVES_AT = "arrives_at";
    private final static String LABEL_OPERATED_BY = "operated_by";
    private final static String LABEL_ROUTE = "route";

    private final static String LABEL_KNOWS = "knows";
    private final static String LABEL_CREATED = "created";
    private final static String LABEL_DEVELOPS = "develops";
    private final static String LABEL_USES = "uses";
    private final static String LABEL_TRAVERSES = "traverses";

    private CacheFactory()
    {
    }

    private static CacheGraph getTinkerpopDefaultCacheGraph()
    {
        final Configuration configuration = getTinkerpopDefaultConfiguration();
        return CacheGraph.open(configuration);
    }

    private static Configuration getTinkerpopDefaultConfiguration()
    {
        final Configuration configuration = new BaseConfiguration();

        configuration.setProperty(
            CacheGraph.CACHEGRAPH_VERTEX_ID_MANAGER,
            CacheGraph.DefaultIdManager.INTEGER.name());
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_EDGE_ID_MANAGER,
            CacheGraph.DefaultIdManager.INTEGER.name());
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_VERTEX_PROPERTY_ID_MANAGER,
            CacheGraph.DefaultIdManager.LONG.name());

        return configuration;
    }

    // Classic graph data set.
    public static CacheGraph createClassic()
    {
        final Configuration configuration = getTinkerpopDefaultConfiguration();

        configuration.setProperty(
            CacheGraph.CACHEGRAPH_VERTEX_PROPERTY_ID_MANAGER,
            CacheGraph.DefaultIdManager.INTEGER.name());

        final CacheGraph graph = CacheGraph.open(configuration);

        generateClassic(graph);

        return graph;
    }

    public static void generateClassic(final CacheGraph graph)
    {
        graph.tx().open();

        final Vertex marko = graph.addVertex(T.id, 1, "name", "marko", "age", 29);
        final Vertex vadas = graph.addVertex(T.id, 2, "name", "vadas", "age", 27);
        final Vertex lop = graph.addVertex(T.id, 3, "name", "lop", "lang", "java");
        final Vertex josh = graph.addVertex(T.id, 4, "name", "josh", "age", 32);
        final Vertex ripple = graph.addVertex(T.id, 5, "name", "ripple", "lang", "java");
        final Vertex peter = graph.addVertex(T.id, 6, "name", "peter", "age", 35);

        marko.addEdge(LABEL_KNOWS, vadas, T.id, 7, "weight", 0.5f);
        marko.addEdge(LABEL_KNOWS, josh, T.id, 8, "weight", 1.0f);
        marko.addEdge(LABEL_CREATED, lop, T.id, 9, "weight", 0.4f);
        josh.addEdge(LABEL_CREATED, ripple, T.id, 10, "weight", 1.0f);
        josh.addEdge(LABEL_CREATED, lop, T.id, 11, "weight", 0.4f);
        peter.addEdge(LABEL_CREATED, lop, T.id, 12, "weight", 0.2f);

        graph.tx().commit();
    }

    // Modern graph data set.
    public static CacheGraph createModern()
    {
        final CacheGraph graph = getTinkerpopDefaultCacheGraph();
        generateModern(graph);
        return graph;
    }

    public static void generateModern(final CacheGraph graph)
    {
        graph.tx().open();

        final Vertex marko = graph.addVertex(T.id, 1, T.label, "person", "name", "marko", "age", 29);
        final Vertex vadas = graph.addVertex(T.id, 2, T.label, "person", "name", "vadas", "age", 27);
        final Vertex lop = graph.addVertex(T.id, 3, T.label, "software", "name", "lop", "lang", "java");
        final Vertex josh = graph.addVertex(T.id, 4, T.label, "person", "name", "josh", "age", 32);
        final Vertex ripple = graph.addVertex(T.id, 5, T.label, "software", "name", "ripple", "lang", "java");
        final Vertex peter = graph.addVertex(T.id, 6, T.label, "person", "name", "peter", "age", 35);

        marko.addEdge(LABEL_KNOWS, vadas, T.id, 7, "weight", 0.5d);
        marko.addEdge(LABEL_KNOWS, josh, T.id, 8, "weight", 1.0d);
        marko.addEdge(LABEL_CREATED, lop, T.id, 9, "weight", 0.4d);
        josh.addEdge(LABEL_CREATED, ripple, T.id, 10, "weight", 1.0d);
        josh.addEdge(LABEL_CREATED, lop, T.id, 11, "weight", 0.4d);
        peter.addEdge(LABEL_CREATED, lop, T.id, 12, "weight", 0.2d);

        graph.tx().commit();
    }

    // The Crew data set.
    public static CacheGraph createTheCrew()
    {
        final Configuration configuration = getTinkerpopDefaultConfiguration();

        configuration.setProperty(
            CacheGraph.CACHEGRAPH_DEFAULT_VERTEX_PROPERTY_CARDINALITY,
            VertexProperty.Cardinality.list.name());

        final CacheGraph graph = CacheGraph.open(configuration);

        generateTheCrew(graph);

        return graph;
    }

    public static void generateTheCrew(final CacheGraph graph)
    {
        graph.tx().open();

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

        marko.addEdge(LABEL_DEVELOPS, gremlin, T.id, 13, "since", 2009);
        marko.addEdge(LABEL_DEVELOPS, tinkergraph, T.id, 14, "since", 2010);
        marko.addEdge(LABEL_USES, gremlin, T.id, 15, "skill", 4);
        marko.addEdge(LABEL_USES, tinkergraph, T.id, 16, "skill", 5);

        stephen.addEdge(LABEL_DEVELOPS, gremlin, T.id, 17, "since", 2010);
        stephen.addEdge(LABEL_DEVELOPS, tinkergraph, T.id, 18, "since", 2011);
        stephen.addEdge(LABEL_USES, gremlin, T.id, 19, "skill", 5);
        stephen.addEdge(LABEL_USES, tinkergraph, T.id, 20, "skill", 4);

        matthias.addEdge(LABEL_DEVELOPS, gremlin, T.id, 21, "since", 2012);
        matthias.addEdge(LABEL_USES, gremlin, T.id, 22, "skill", 3);
        matthias.addEdge(LABEL_USES, tinkergraph, T.id, 23, "skill", 3);

        daniel.addEdge(LABEL_USES, gremlin, T.id, 24, "skill", 5);
        daniel.addEdge(LABEL_USES, tinkergraph, T.id, 25, "skill", 3);

        gremlin.addEdge(LABEL_TRAVERSES, tinkergraph, T.id, 26);

        graph.variables().set("creator", "marko");
        graph.variables().set("lastModified", 2014);
        graph.variables().set("comment", "this graph was created to provide examples and test coverage for tinkerpop3 api advances");

        graph.tx().commit();
    }

    // Kitchen Sink data set.
    public static CacheGraph createKitchenSink()
    {
        final CacheGraph graph = getTinkerpopDefaultCacheGraph();
        generateKitchenSink(graph);
        return graph;
    }

    public static void generateKitchenSink(final CacheGraph graph)
    {
        graph.tx().open();

        final GraphTraversalSource g = graph.traversal();

        g.addV("loops").property(T.id, 1000).property("name", "loop").as("me")
            .addE("self").to("me").property(T.id, 1001)
            .iterate();

        g.addV("message").property(T.id, 2000).property("name", "a").as("a")
            .addV("message").property(T.id, 2001).property("name", "b").as("b")
            .addE("link").from("a").to("b").property(T.id, 2002)
            .addE("link").from("a").to("a").property(T.id, 2003).iterate();

        graph.tx().commit();
    }

    private static CacheGraph getDefaultCacheGraph()
    {
        final Configuration configuration = getDefaultConfiguration();
        return CacheGraph.open(configuration);
    }

    private static Configuration getDefaultConfiguration()
    {
        final Configuration configuration = new BaseConfiguration();

        configuration.setProperty(
            CacheGraph.CACHEGRAPH_VERTEX_ID_MANAGER,
            CacheGraph.DefaultIdManager.LONG.name());
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_EDGE_ID_MANAGER,
            CacheGraph.DefaultIdManager.LONG.name());
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_VERTEX_PROPERTY_ID_MANAGER,
            CacheGraph.DefaultIdManager.LONG.name());

        return configuration;
    }

    // Tiny airport data set.
    public static CacheGraph createTinyAirport()
    {
        final CacheGraph graph = getDefaultCacheGraph();
        generateTinyAirport(graph);
        return graph;
    }

    private static void setFlightEdges(Vertex airline, Vertex departureAirport, Vertex arrivalAirport, Vertex flight)
    {
        departureAirport.addEdge(LABEL_DEPARTURE, flight);
        flight.addEdge(LABEL_ARRIVES_AT, arrivalAirport);
        flight.addEdge(LABEL_OPERATED_BY, airline);
    }

    public static void generateTinyAirport(final CacheGraph graph)
    {
        graph.tx().open();

        final Vertex ams = graph.addVertex(T.label, LABEL_AIRPORT,
            "id", "580", "iata", "AMS", "name", "Amsterdam Airport Schiphol", "city", "Amsterdam", "country", "NLD");
        final Vertex cdg = graph.addVertex(T.label, LABEL_AIRPORT,
            "id", "1382", "iata", "CDG", "name", "Charles de Gaulle International Airport", "city", "Paris", "country", "FRA");
        final Vertex jfk = graph.addVertex(T.label, LABEL_AIRPORT,
            "id", "3797", "iata", "JFK", "name", "John F Kennedy International Airport", "city", "New York", "country", "USA");
        final Vertex otp = graph.addVertex(T.label, LABEL_AIRPORT,
            "id", "1657", "iata", "OTP", "name", "Henri Coandă International Airport", "city", "Bucharest", "country", "ROU");
        final Vertex sea = graph.addVertex(T.label, LABEL_AIRPORT,
            "id", "3577", "iata", "SEA", "name", "Seattle Tacoma International Airport", "city", "Seattle", "country", "USA");

        final Vertex aal = graph.addVertex(T.label, LABEL_AIRLINE,
            "id", "24", "icao", "AAL", "name", "American Airlines");
        final Vertex afr = graph.addVertex(T.label, LABEL_AIRLINE,
            "id", "137", "icao", "AFR", "name", "Air France");
        final Vertex dal = graph.addVertex(T.label, LABEL_AIRLINE,
            "id", "2009", "icao", "DAL", "name", "Delta Air Lines");
        final Vertex klm = graph.addVertex(T.label, LABEL_AIRLINE,
            "id", "3090", "icao", "KLM", "name", "KLM Royal Dutch Airlines");

        final Vertex aal_cdg_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "767-300 757");
        final Vertex aal_jfk_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "767-300 757");
        final Vertex aal_jfk_sea = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "737-800");
        final Vertex aal_sea_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "737-800");

        final Vertex afr_ams_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A321 A320 A319 A318 A320(s)");
        final Vertex afr_cdg_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A321 A320 A319 A318 A320(s)");
        final Vertex afr_cdg_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200 777-200 A380-800 A340-300");
        final Vertex afr_cdg_otp = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A321 A320 A319");
        final Vertex afr_cdg_sea = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200");
        final Vertex afr_jfk_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200 777-200 A380-800 A340-300");
        final Vertex afr_otp_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A321 A320 A319");
        final Vertex afr_sea_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200");

        final Vertex dal_ams_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "767-300(w)");
        final Vertex dal_ams_sea = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-300");
        final Vertex dal_cdg_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200 777-200 A380-800 A340-300");
        final Vertex dal_cdg_sea = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200");
        final Vertex dal_jfk_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "767-300(w)");
        final Vertex dal_jfk_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200 777-200 A380-800 A340-300");
        final Vertex dal_jfk_sea = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "757-200 757");
        final Vertex dal_sea_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-300");
        final Vertex dal_sea_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330-200");
        final Vertex dal_sea_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "757-200 757");

        final Vertex klm_ams_cdg = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "737");
        final Vertex klm_ams_jfk = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330 777 747(Combi) 747");
        final Vertex klm_ams_otp = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "737");
        final Vertex klm_ams_sea = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330");
        final Vertex klm_cdg_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "737");
        final Vertex klm_jfk_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330 777 747(Combi) 747");
        final Vertex klm_otp_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "737");
        final Vertex klm_sea_ams = graph.addVertex(T.label, LABEL_FLIGHT, "equipment", "A330");

        setFlightEdges(aal, cdg, jfk, aal_cdg_jfk);
        setFlightEdges(aal, jfk, sea, aal_jfk_sea);
        setFlightEdges(aal, sea, jfk, aal_sea_jfk);

        setFlightEdges(afr, ams, cdg, afr_ams_cdg);
        setFlightEdges(afr, cdg, ams, afr_cdg_ams);
        setFlightEdges(afr, cdg, jfk, afr_cdg_jfk);
        setFlightEdges(afr, cdg, otp, afr_cdg_otp);
        setFlightEdges(afr, cdg, sea, afr_cdg_sea);
        setFlightEdges(afr, jfk, cdg, afr_jfk_cdg);
        setFlightEdges(afr, otp, cdg, afr_otp_cdg);
        setFlightEdges(afr, sea, cdg, afr_sea_cdg);

        setFlightEdges(dal, ams, jfk, dal_ams_jfk);
        setFlightEdges(dal, ams, sea, dal_ams_sea);
        setFlightEdges(dal, cdg, jfk, dal_cdg_jfk);
        setFlightEdges(dal, cdg, sea, dal_cdg_sea);
        setFlightEdges(dal, jfk, ams, dal_jfk_ams);
        setFlightEdges(dal, jfk, cdg, dal_jfk_cdg);
        setFlightEdges(dal, jfk, sea, dal_jfk_sea);
        setFlightEdges(dal, sea, ams, dal_sea_ams);
        setFlightEdges(dal, sea, cdg, dal_sea_cdg);
        setFlightEdges(dal, sea, jfk, dal_sea_jfk);

        setFlightEdges(klm, ams, cdg, klm_ams_cdg);
        setFlightEdges(klm, ams, jfk, klm_ams_jfk);
        setFlightEdges(klm, ams, otp, klm_ams_otp);
        setFlightEdges(klm, ams, sea, klm_ams_sea);
        setFlightEdges(klm, cdg, ams, klm_cdg_ams);
        setFlightEdges(klm, jfk, ams, klm_jfk_ams);
        setFlightEdges(klm, otp, ams, klm_otp_ams);
        setFlightEdges(klm, sea, ams, klm_sea_ams);

        graph.tx().commit();
    }

    // Tiny airport data set with Q1 schema.
    public static CacheGraph createTinyQ1Airport()
    {
        final CacheGraph graph = getDefaultCacheGraph();
        generateTinyQ1Airport(graph);
        return graph;
    }

    private static void setRouteEdge(Vertex departureAirport, Vertex arrivalAirport, String airline, String equipment)
    {
        departureAirport.addEdge(LABEL_ROUTE, arrivalAirport, "airline", airline, "equipment", equipment);
        arrivalAirport.addEdge(LABEL_ROUTE, departureAirport, "airline", airline, "equipment", equipment);
    }

    public static void generateTinyQ1Airport(final CacheGraph graph)
    {
        graph.tx().open();

        final Vertex aal = graph.addVertex(T.label, LABEL_AIRLINE,
            "al_id", "24", "iata", "AA", "icao", "AAL", "name", "American Airlines");
        final Vertex afr = graph.addVertex(T.label, LABEL_AIRLINE,
            "al_id", "137", "iata", "AF", "icao", "AFR", "name", "Air France");
        final Vertex dal = graph.addVertex(T.label, LABEL_AIRLINE,
            "al_id", "2009", "iata", "DL", "icao", "DAL", "name", "Delta Air Lines");
        final Vertex klm = graph.addVertex(T.label, LABEL_AIRLINE,
            "al_id", "3090", "iata", "KL", "icao", "KLM", "name", "KLM Royal Dutch Airlines");

        final Vertex ams = graph.addVertex(T.label, LABEL_AIRPORT,
            "ap_id", "580", "iata", "AMS", "name", "Amsterdam Airport Schiphol", "city", "Amsterdam", "country", "NLD");
        final Vertex cdg = graph.addVertex(T.label, LABEL_AIRPORT,
            "ap_id", "1382", "iata", "CDG", "name", "Charles de Gaulle International Airport", "city", "Paris", "country", "FRA");
        final Vertex jfk = graph.addVertex(T.label, LABEL_AIRPORT,
            "ap_id", "3797", "iata", "JFK", "name", "John F Kennedy International Airport", "city", "New York", "country", "USA");
        final Vertex otp = graph.addVertex(T.label, LABEL_AIRPORT,
            "ap_id", "1657", "iata", "OTP", "name", "Henri Coandă International Airport", "city", "Bucharest", "country", "ROU");
        final Vertex sea = graph.addVertex(T.label, LABEL_AIRPORT,
            "ap_id", "3577", "iata", "SEA", "name", "Seattle Tacoma International Airport", "city", "Seattle", "country", "USA");

        setRouteEdge(cdg, jfk, "AA", "767-300 757");
        setRouteEdge(jfk, sea, "AA", "737-800");

        setRouteEdge(ams, cdg, "AF", "A321 A320 A319 A318 A320(s)");
        setRouteEdge(cdg, jfk, "AF", "A330-200 777-200 A380-800 A340-300");
        setRouteEdge(cdg, otp, "AF", "A321 A320 A319");
        setRouteEdge(cdg, sea, "AF", "A330-200");

        setRouteEdge(ams, jfk, "DL", "767-300(w)");
        setRouteEdge(ams, sea, "DL", "A330-300");
        setRouteEdge(cdg, jfk, "DL", "A330-200 777-200 A380-800 A340-300");
        setRouteEdge(cdg, sea, "DL", "A330-200");
        setRouteEdge(jfk, sea, "DL", "757-200 757");
        setRouteEdge(ams, jfk, "DL", "767-300(w)");

        setRouteEdge(ams, cdg, "KL", "737");
        setRouteEdge(ams, jfk, "KL", "A330 777 747(Combi) 747");
        setRouteEdge(ams, otp, "KL", "737");
        setRouteEdge(ams, sea, "KL", "A330");

        graph.tx().commit();
    }

    public static void enableDebuggingMessages(final CacheGraph graph, boolean enabled)
    {
        graph.enableDebugMessages = enabled;
    }

    // Load a graphml file.
    public static CacheGraph readGraphml(String filename)
    {
        final CacheGraph graph = getDefaultCacheGraph();
        readGraphml(graph, filename);
        return graph;
    }

    public static boolean readGraphml(final CacheGraph graph, String filename)
    {
        // If transactions are advertised to be supported (via Graph.Features)
        // then readGraph() will call commit() and rollback() itself,
        // but it still expects open() to have been called already.
        //
        // readGraph() may also fail if it attempts to batch transactions,
        // because it doesn't re-open a transaction after a commit.
        //
        // For now, we don't advertise transaction support,
        // so we have to call commit() and rollback() ourselves.
        graph.tx().open();
        try
        {
            graph.io(graphml()).readGraph(filename);
            graph.tx().commit();

            return true;
        }
        catch (Exception e)
        {
            graph.tx().rollback();
            System.out.println(
                "An error happened while attempting to load " + filename + ": "
                + e.getMessage());

            return false;
        }
    }

    // Write graph data to a graphml file.
    public static boolean writeGraphml(final CacheGraph graph, String filename)
    {
        try
        {
            final File file = new File(filename);
            final File parentFile = file.getParentFile();
            if (parentFile != null && !parentFile.exists())
            {
                parentFile.mkdirs();
            }

            graph.io(graphml()).writeGraph(filename);

            return true;
        }
        catch (Exception e)
        {
            System.out.println(
                "An error happened while attempting to load " + filename + ": "
                + e.getMessage());

            return false;
        }
    }

    // Open CacheGraph instance without Gaia database writing.
    // This is useful for testing the in-memory graph part only.
    public static CacheGraph openWithoutGaiaDb()
    {
        final Configuration configuration = getDefaultConfiguration();

        // Disable Gaia database operations.
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_ENABLE_GAIADB_OPERATIONS,
            false);

        final CacheGraph graph = CacheGraph.open(configuration);

        return graph;
    }

    // Open CacheGraph instance with airport serialization to Gaia database.
    // This is useful for testing airport data serialization to Gaia database.
    public static CacheGraph openWithAirportSupport()
    {
        final Configuration configuration = getDefaultConfiguration();

        // Enable airport data serialization code.
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_ENABLE_AIRPORT_CODE,
            true);

        final CacheGraph graph = CacheGraph.open(configuration);

        return graph;
    }

    // A method for loading airport data from Gaia datadase.
    public static CacheGraph loadAirportGraphFromGaiaDb()
    {
        final Configuration configuration = getDefaultConfiguration();

        // We need to load data, so do not initialize Gaia database.
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_CREATE_ON_START,
            false);

        // We read from Gaia database to write into cached graph,
        // so we don't need to write back into Gaia database.
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_ENABLE_GAIADB_OPERATIONS,
            false);

        // We need to enable airport data serialization code.
        configuration.setProperty(
            CacheGraph.CACHEGRAPH_ENABLE_AIRPORT_CODE,
            true);

        final CacheGraph graph = CacheGraph.open(configuration);

        return graph;
    }
}
