/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/////////////////////////////////////////////
// Portions of this code are derived
// from TinkerGraph project.
// Used under Apache License 2.0
/////////////////////////////////////////////

package com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.T;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

import com.google.flatbuffers.FlatBufferBuilder;

import com.gaiaplatform.database.GaiaDatabase;
import com.gaiaplatform.database.Airline;
import com.gaiaplatform.database.Airport;
import com.gaiaplatform.database.Route;

public final class CacheHelper
{
    public final static String EMPTY_STRING = "";

    private final static String PROPERTY_DELIMITER = "|";
    private final static String KEY_VALUE_DELIMITER = "=";

    private final static String PROPERTY_AL_ID = "al_id";
    private final static String PROPERTY_NAME = "name";
    private final static String PROPERTY_ALIAS = "alias";
    private final static String PROPERTY_IATA = "iata";
    private final static String PROPERTY_ICAO = "icao";
    private final static String PROPERTY_CALLSIGN = "callsign";
    private final static String PROPERTY_COUNTRY = "country";
    private final static String PROPERTY_ACTIVE = "active";
    private final static String PROPERTY_AP_ID = "ap_id";
    private final static String PROPERTY_CITY = "city";
    private final static String PROPERTY_LATITUDE = "latitude";
    private final static String PROPERTY_LONGITUDE = "longitude";
    private final static String PROPERTY_ALTITUDE = "altitude";
    private final static String PROPERTY_TIMEZONE = "timezone";
    private final static String PROPERTY_DST = "dst";
    private final static String PROPERTY_TZTEXT = "tztext";
    private final static String PROPERTY_TYPE = "type";
    private final static String PROPERTY_SOURCE = "source";
    private final static String PROPERTY_AIRLINE = "airline";
    private final static String PROPERTY_SRC_AP = "src_ap";
    private final static String PROPERTY_SRC_AP_ID = "src_ap_id";
    private final static String PROPERTY_DST_AP = "dst_ap";
    private final static String PROPERTY_DST_AP_ID = "dst_ap_id";
    private final static String PROPERTY_CODESHARE = "codeshare";
    private final static String PROPERTY_STOPS = "stops";
    private final static String PROPERTY_EQUIPMENT = "equipment";

    private final static long AIRLINE_NODE_TYPE = 1;
    private final static long AIRPORT_NODE_TYPE = 2;
    private final static long ROUTE_EDGE_TYPE = 3;

    private static AtomicLong lastType = new AtomicLong();
    private static Map<String, Long> mapLabelsToTypes = new ConcurrentHashMap<>();

    private CacheHelper()
    {
    }

    protected static void reset()
    {
        lastType.set(0);
        mapLabelsToTypes.clear();
    }

    protected static void debugPrint(CacheGraph graph, String message)
    {
        if (graph.enableDebugMessages)
        {
            System.out.println(message);
        }
    }

    public static void printStackTrace()
    {
        System.out.println("\n>>> BEGIN STACK TRACE:\n");

        StackTraceElement[] stackElements = Thread.currentThread().getStackTrace();
        for (int i = 1; i < stackElements.length; i++)
        {
            System.out.println("--> " + stackElements[i]);
        }

        System.out.println("\n<<< END STACK TRACE\n");
    }

    // This makes CacheGraph load its status from the underlying store,
    // whose schema is expected to follow a specific implementation.
    protected static void loadAirportGraphFromGaiaDb(CacheGraph graph)
    {
        if (graph.enableGaiaDbOperations)
        {
            System.out.println("Gaia database writes need to be disabled while data is loaded from it. Aborting load!");
            return;
        }

        if (!graph.enableAirportCode)
        {
            System.out.println("Gaia database airport code should be enabled. Aborting load!");
            return;
        }

        reset();

        // Gaia database operations are disabled, so we need to manually create the transaction.
        graph.gaiaDb.beginTransaction();

        // Scan airline nodes.
        long currentNodeId = graph.gaiaDb.findFirstNode(AIRLINE_NODE_TYPE);
        int count = 0;
        for (count = 0;
            currentNodeId != 0;
            currentNodeId = graph.gaiaDb.findNextNode(currentNodeId), ++count)
        {
            System.out.print("Loading airline node " + currentNodeId + "...");
            loadAirlineNode(graph, currentNodeId);
            System.out.println("done!");
        }

        System.out.println("" + count + " airline nodes have been loaded!\n");

        // Scan airport nodes.
        currentNodeId = graph.gaiaDb.findFirstNode(AIRPORT_NODE_TYPE);
        for (count = 0;
            currentNodeId != 0;
            currentNodeId = graph.gaiaDb.findNextNode(currentNodeId), ++count)
        {
            System.out.print("Loading airport node " + currentNodeId + "...");
            loadAirportNode(graph, currentNodeId);
            System.out.println("done!");
        }

        System.out.println("" + count + " airport nodes have been loaded!\n");

        // Scan route edges.
        long currentEdgeId = graph.gaiaDb.findFirstEdge(ROUTE_EDGE_TYPE);
        for (count = 0;
            currentEdgeId != 0;
            currentEdgeId = graph.gaiaDb.findNextEdge(currentEdgeId), ++count)
        {
            System.out.print("Loading route edge " + currentEdgeId + "...");
            loadRouteEdge(graph, currentEdgeId);
            System.out.println("done!");
        }

        System.out.println("" + count + " route edges have been loaded!\n");

        // We're not making any changes to Gaia database, so we can just rollback.
        graph.gaiaDb.rollbackTransaction();

        // We're done loading data into in-memory cache,
        // so we can re-enable writes to our store.
        graph.enableGaiaDbOperations = true;
    }

    private static void loadAirlineNode(CacheGraph graph, long nodeId)
    {
        List<Object> keyValues = new ArrayList<>();
        keyValues.add(T.id);
        keyValues.add(nodeId);
        keyValues.add(T.label);
        keyValues.add("airline");

        byte[] payload = graph.gaiaDb.getNodePayload(nodeId);
        if (payload != null)
        {
            java.nio.ByteBuffer byteBuffer = java.nio.ByteBuffer.wrap(payload);

            Airline airline = null;
            try
            {
                airline = Airline.getRootAsAirline(byteBuffer);
            }
            catch (Exception e)
            {
                System.out.println("Flatbuffers deserialization failed! Will ignore payload data.");
            }

            if (airline != null)
            {
                if (airline.alId() != 0)
                {
                    keyValues.add(PROPERTY_AL_ID);
                    keyValues.add(airline.alId());
                }
                if (airline.name() != null)
                {
                    keyValues.add(PROPERTY_NAME);
                    keyValues.add(airline.name());
                }
                if (airline.alias() != null)
                {
                    keyValues.add(PROPERTY_ALIAS);
                    keyValues.add(airline.alias());
                }
                if (airline.iata() != null)
                {
                    keyValues.add(PROPERTY_IATA);
                    keyValues.add(airline.iata());
                }
                if (airline.icao() != null)
                {
                    keyValues.add(PROPERTY_ICAO);
                    keyValues.add(airline.icao());
                }
                if (airline.callsign() != null)
                {
                    keyValues.add(PROPERTY_CALLSIGN);
                    keyValues.add(airline.callsign());
                }
                if (airline.country() != null)
                {
                    keyValues.add(PROPERTY_COUNTRY);
                    keyValues.add(airline.country());
                }
                if (airline.active() != null)
                {
                    keyValues.add(PROPERTY_ACTIVE);
                    keyValues.add(airline.active());
                }
            }
        }

        graph.addVertex(keyValues.toArray());
    }

    private static void loadAirportNode(CacheGraph graph, long nodeId)
    {
        List<Object> keyValues = new ArrayList<>();
        keyValues.add(T.id);
        keyValues.add(nodeId);
        keyValues.add(T.label);
        keyValues.add("airport");

        byte[] payload = graph.gaiaDb.getNodePayload(nodeId);
        if (payload != null)
        {
            java.nio.ByteBuffer byteBuffer = java.nio.ByteBuffer.wrap(payload);

            Airport airport = null;
            try
            {
                airport = Airport.getRootAsAirport(byteBuffer);
            }
            catch (Exception e)
            {
                System.out.println("Flatbuffers deserialization failed! Will ignore payload data.");
            }

            if (airport != null)
            {
                if (airport.apId() != 0)
                {
                    keyValues.add(PROPERTY_AP_ID);
                    keyValues.add(airport.apId());
                }
                if (airport.name() != null)
                {
                    keyValues.add(PROPERTY_NAME);
                    keyValues.add(airport.name());
                }
                if (airport.city() != null)
                {
                    keyValues.add(PROPERTY_CITY);
                    keyValues.add(airport.city());
                }
                if (airport.country() != null)
                {
                    keyValues.add(PROPERTY_COUNTRY);
                    keyValues.add(airport.country());
                }
                if (airport.iata() != null)
                {
                    keyValues.add(PROPERTY_IATA);
                    keyValues.add(airport.iata());
                }
                if (airport.icao() != null)
                {
                    keyValues.add(PROPERTY_ICAO);
                    keyValues.add(airport.icao());
                }
                if (airport.latitude() != 0)
                {
                    keyValues.add(PROPERTY_LATITUDE);
                    keyValues.add(airport.latitude());
                }
                if (airport.longitude() != 0)
                {
                    keyValues.add(PROPERTY_LONGITUDE);
                    keyValues.add(airport.longitude());
                }
                keyValues.add(PROPERTY_ALTITUDE);
                keyValues.add(airport.altitude());
                keyValues.add(PROPERTY_TIMEZONE);
                keyValues.add(airport.timezone());
                if (airport.dst() != null)
                {
                    keyValues.add(PROPERTY_DST);
                    keyValues.add(airport.dst());
                }
                if (airport.tztext() != null)
                {
                    keyValues.add(PROPERTY_TZTEXT);
                    keyValues.add(airport.tztext());
                }
                if (airport.type() != null)
                {
                    keyValues.add(PROPERTY_TYPE);
                    keyValues.add(airport.type());
                }
                if (airport.source() != null)
                {
                    keyValues.add(PROPERTY_SOURCE);
                    keyValues.add(airport.source());
                }
            }
        }

        graph.addVertex(keyValues.toArray());
    }

    private static void loadRouteEdge(CacheGraph graph, long edgeId)
    {
        List<Object> keyValues = new ArrayList<>();
        keyValues.add(T.id);
        keyValues.add(edgeId);

        byte[] payload = graph.gaiaDb.getEdgePayload(edgeId);
        if (payload != null)
        {
            java.nio.ByteBuffer byteBuffer = java.nio.ByteBuffer.wrap(payload);

            Route route = null;
            try
            {
                route = Route.getRootAsRoute(byteBuffer);
            }
            catch (Exception e)
            {
                System.out.println("Flatbuffers deserialization failed! Will ignore payload data.");
            }

            if (route != null)
            {
                if (route.airline() != null)
                {
                    keyValues.add(PROPERTY_AIRLINE);
                    keyValues.add(route.airline());
                }
                if (route.alId() != 0)
                {
                    keyValues.add(PROPERTY_AL_ID);
                    keyValues.add(route.alId());
                }
                if (route.srcAp() != null)
                {
                    keyValues.add(PROPERTY_SRC_AP);
                    keyValues.add(route.srcAp());
                }
                if (route.srcApId() != 0)
                {
                    keyValues.add(PROPERTY_SRC_AP_ID);
                    keyValues.add(route.srcApId());
                }
                if (route.dstAp() != null)
                {
                    keyValues.add(PROPERTY_DST_AP);
                    keyValues.add(route.dstAp());
                }
                if (route.dstApId() != 0)
                {
                    keyValues.add(PROPERTY_DST_AP_ID);
                    keyValues.add(route.dstApId());
                }
                if (route.codeshare() != null)
                {
                    keyValues.add(PROPERTY_CODESHARE);
                    keyValues.add(route.codeshare());
                }
                keyValues.add(PROPERTY_STOPS);
                keyValues.add(route.stops());
                if (route.equipment() != null)
                {
                    keyValues.add(PROPERTY_EQUIPMENT);
                    keyValues.add(route.equipment());
                }
            }
        }

        long idFirstNode = graph.gaiaDb.getEdgeFirstNode(edgeId);
        long idSecondNode = graph.gaiaDb.getEdgeSecondNode(edgeId);

        if (idFirstNode == 0
            || idSecondNode == 0
            || !graph.vertices.containsKey(idFirstNode)
            || !graph.vertices.containsKey(idSecondNode))
        {
            System.out.println(
                "Cannot resolve edge node ids: first=" + idFirstNode
                + ", second=" + idSecondNode + ". Will skip route edge!");
        }

        Vertex outVertex = graph.vertices.get(idFirstNode);
        Vertex inVertex = graph.vertices.get(idSecondNode);

        outVertex.addEdge("route", inVertex, keyValues.toArray());
    }

    // Because the underlying store does not support string labels or metadata,
    // we need to map strings to integer values for operations with the underlying store.
    private static long getTypeForLabel(String label)
    {
        if (mapLabelsToTypes.containsKey(label))
        {
            return mapLabelsToTypes.get(label).longValue();
        }

        long nextType = lastType.incrementAndGet();
        mapLabelsToTypes.put(label, nextType);

        System.out.println("Mapped label [" + label + "] to type [" + nextType + "].");

        return nextType;
    }

    // Returns true if we started a transaction and false if one was already started.
    private static boolean startTransaction(CacheGraph graph)
    {
        if (graph.tx().isOpen())
        {
            return false;
        }

        graph.tx().open();
        return true;
    }

    // Performs a commit or rollback, according to the operation result
    // and to whether we were the ones to open the transaction.
    private static boolean handleTransaction(CacheGraph graph, boolean operationResult, boolean ownTransaction)
    {
        if (ownTransaction)
        {
            if (operationResult)
            {
                graph.tx().commit();
            }
            else
            {
                graph.tx().rollback();
            }
        }

        return operationResult;
    }

    // Underlying store helpers.
    // These are called through the in-memory graph update operations
    // and they update the underlying store.
    protected static boolean createNode(CacheVertex vertex)
    {
        debugPrint(vertex.graph, "helper::createNode(" + vertex.id + ")");

        if (!vertex.graph.enableGaiaDbOperations)
        {
            return true;
        }

        if (vertex.graph.enableAirportCode)
        {
            return createAirportNode(vertex);
        }
        else
        {
            return createGenericNode(vertex);
        }
    }

    protected static boolean removeNode(CacheVertex vertex)
    {
        debugPrint(vertex.graph, "helper::removeNode(" + vertex.id + ")");

        if (!vertex.graph.enableGaiaDbOperations)
        {
            return true;
        }

        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());

        boolean ownTransaction = startTransaction(graph);
        return handleTransaction(graph, graph.gaiaDb.removeNode(id), ownTransaction);
    }

    protected static boolean updateNodePayload(CacheVertex vertex)
    {
        debugPrint(vertex.graph, "helper::updateNodePayload(" + vertex.id + ")");

        if (!vertex.graph.enableGaiaDbOperations)
        {
            return true;
        }

        if (vertex.graph.enableAirportCode)
        {
            return updateAirportNodePayload(vertex);
        }
        else
        {
            return updateGenericNodePayload(vertex);
        }
    }

    protected static boolean createEdge(CacheEdge edge)
    {
        debugPrint(edge.graph, "helper::createEdge(" + edge.id + ")");

        if (!edge.graph.enableGaiaDbOperations)
        {
            return true;
        }

        if (edge.graph.enableAirportCode)
        {
            return createAirportEdge(edge);
        }
        else
        {
            return createGenericEdge(edge);
        }
    }

    protected static boolean removeEdge(CacheEdge edge)
    {
        debugPrint(edge.graph, "helper::removeEdge(" + edge.id + ")");

        if (!edge.graph.enableGaiaDbOperations)
        {
            return true;
        }

        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());

        boolean ownTransaction = startTransaction(graph);
        return handleTransaction(graph, graph.gaiaDb.removeEdge(id), ownTransaction);
    }

    protected static boolean updateEdgePayload(CacheEdge edge)
    {
        debugPrint(edge.graph, "helper::updateEdgePayload(" + edge.id + ")");

        if (!edge.graph.enableGaiaDbOperations)
        {
            return true;
        }

        if (edge.graph.enableAirportCode)
        {
            return updateAirportEdgePayload(edge);
        }
        else
        {
            return updateGenericEdgePayload(edge);
        }
    }

    // Generic graph helpers that do a simple string serialization of properties.
    private static void packProperty(StringBuilder payload, String key, String value)
    {
        if (payload.length() > 0)
        {
            payload.append(PROPERTY_DELIMITER);
        }

        payload.append(key);
        payload.append(KEY_VALUE_DELIMITER);
        payload.append(value);
    }

    private static String packNodeProperties(Map<String, List<VertexProperty>> properties)
    {
        if (properties == null)
        {
            return EMPTY_STRING;
        }

        StringBuilder payload = new StringBuilder();

        properties.forEach((key, list) -> packProperty(payload, key, list.get(0).value().toString()));

        return payload.toString();
    }

    private static String packEdgeProperties(Map<String, Property> properties)
    {
        if (properties == null)
        {
            return EMPTY_STRING;
        }

        StringBuilder payload = new StringBuilder();

        properties.forEach((key, property) -> packProperty(payload, key, property.value().toString()));

        return payload.toString();
    }

    protected static boolean createGenericNode(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        long type = getTypeForLabel(vertex.label);
        String payload = packNodeProperties(vertex.properties);

        boolean ownTransaction = startTransaction(graph);
        long idNode = graph.gaiaDb.createNode(id, type, payload);
        return handleTransaction(graph, idNode != 0, ownTransaction);
    }

    protected static boolean updateGenericNodePayload(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        String payload = packNodeProperties(vertex.properties);

        boolean ownTransaction = startTransaction(graph);
        return handleTransaction(graph, graph.gaiaDb.updateNodePayload(id, payload), ownTransaction);
    }

    protected static boolean createGenericEdge(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        long type = getTypeForLabel(edge.label);
        String payload = packEdgeProperties(edge.properties);
        long idFirstNode = Long.parseLong(edge.outVertex.id.toString());
        long idSecondNode = Long.parseLong(edge.inVertex.id.toString());

        boolean ownTransaction = startTransaction(graph);
        long idEdge = graph.gaiaDb.createEdge(id, type, idFirstNode, idSecondNode, payload);
        return handleTransaction(graph, idEdge != 0, ownTransaction);
    }

    protected static boolean updateGenericEdgePayload(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        String payload = packEdgeProperties(edge.properties);

        boolean ownTransaction = startTransaction(graph);
        return handleTransaction(graph, graph.gaiaDb.updateEdgePayload(id, payload), ownTransaction);
    }

    // Airport graph helpers that do a custom flatbuffers serialization of properties.
    private static byte[] packAirportNodeProperties(long type, Map<String, List<VertexProperty>> properties)
    {
        if (properties == null)
        {
            return null;
        }

        FlatBufferBuilder builder = new FlatBufferBuilder();

        int nameOffset = 0;
        int aliasOffset = 0;
        int iataOffset = 0;
        int icaoOffset = 0;
        int callsignOffset = 0;
        int countryOffset = 0;
        int activeOffset = 0;
        int cityOffset = 0;
        int dstOffset = 0;
        int tztextOffset = 0;
        int typeOffset = 0;
        int sourceOffset = 0;

        if (type == AIRLINE_NODE_TYPE)
        {
            // Serialize string fields before serializing the entire object.
            if (properties.containsKey(PROPERTY_NAME))
            {
                nameOffset = builder.createString(properties.get(PROPERTY_NAME).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_ALIAS))
            {
                aliasOffset = builder.createString(properties.get(PROPERTY_ALIAS).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_IATA))
            {
                iataOffset = builder.createString(properties.get(PROPERTY_IATA).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_ICAO))
            {
                icaoOffset = builder.createString(properties.get(PROPERTY_ICAO).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_CALLSIGN))
            {
                callsignOffset = builder.createString(properties.get(PROPERTY_CALLSIGN).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_COUNTRY))
            {
                countryOffset = builder.createString(properties.get(PROPERTY_COUNTRY).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_ACTIVE))
            {
                activeOffset = builder.createString(properties.get(PROPERTY_ACTIVE).get(0).value().toString());
            }

            Airline.startAirline(builder);

            if (properties.containsKey(PROPERTY_AL_ID))
            {
                int alId = Integer.parseInt(properties.get(PROPERTY_AL_ID).get(0).value().toString());
                Airline.addAlId(builder, alId);
            }
            if (properties.containsKey(PROPERTY_NAME))
            {
                Airline.addName(builder, nameOffset);
            }
            if (properties.containsKey(PROPERTY_ALIAS))
            {
                Airline.addAlias(builder, aliasOffset);
            }
            if (properties.containsKey(PROPERTY_IATA))
            {
                Airline.addIata(builder, iataOffset);
            }
            if (properties.containsKey(PROPERTY_ICAO))
            {
                Airline.addIcao(builder, icaoOffset);
            }
            if (properties.containsKey(PROPERTY_CALLSIGN))
            {
                Airline.addCallsign(builder, callsignOffset);
            }
            if (properties.containsKey(PROPERTY_COUNTRY))
            {
                Airline.addCountry(builder, countryOffset);
            }
            if (properties.containsKey(PROPERTY_ACTIVE))
            {
                Airline.addName(builder, activeOffset);
            }

            int airlineOffset = Airline.endAirline(builder);
            builder.finish(airlineOffset);
        }
        else if (type == AIRPORT_NODE_TYPE)
        {
            // Serialize string fields before serializing the entire object.
            if (properties.containsKey(PROPERTY_NAME))
            {
                nameOffset = builder.createString(properties.get(PROPERTY_NAME).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_CITY))
            {
                cityOffset = builder.createString(properties.get(PROPERTY_CITY).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_COUNTRY))
            {
                countryOffset = builder.createString(properties.get(PROPERTY_COUNTRY).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_IATA))
            {
                iataOffset = builder.createString(properties.get(PROPERTY_IATA).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_ICAO))
            {
                icaoOffset = builder.createString(properties.get(PROPERTY_ICAO).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_DST))
            {
                dstOffset = builder.createString(properties.get(PROPERTY_DST).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_TZTEXT))
            {
                tztextOffset = builder.createString(properties.get(PROPERTY_TZTEXT).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_TYPE))
            {
                typeOffset = builder.createString(properties.get(PROPERTY_TYPE).get(0).value().toString());
            }
            if (properties.containsKey(PROPERTY_SOURCE))
            {
                sourceOffset = builder.createString(properties.get(PROPERTY_SOURCE).get(0).value().toString());
            }

            Airport.startAirport(builder);

            if (properties.containsKey(PROPERTY_AP_ID))
            {
                int apId = Integer.parseInt(properties.get(PROPERTY_AP_ID).get(0).value().toString());
                Airport.addApId(builder, apId);
            }
            if (properties.containsKey(PROPERTY_NAME))
            {
                Airport.addName(builder, nameOffset);
            }
            if (properties.containsKey(PROPERTY_CITY))
            {
                Airport.addCity(builder, cityOffset);
            }
            if (properties.containsKey(PROPERTY_COUNTRY))
            {
                Airport.addCountry(builder, countryOffset);
            }
            if (properties.containsKey(PROPERTY_IATA))
            {
                Airport.addIata(builder, iataOffset);
            }
            if (properties.containsKey(PROPERTY_ICAO))
            {
                Airport.addIcao(builder, icaoOffset);
            }
            if (properties.containsKey(PROPERTY_LATITUDE))
            {
                double latitude = Double.parseDouble(properties.get(PROPERTY_LATITUDE).get(0).value().toString());
                Airport.addLatitude(builder, latitude);
            }
            if (properties.containsKey(PROPERTY_LONGITUDE))
            {
                double longitude = Double.parseDouble(properties.get(PROPERTY_LONGITUDE).get(0).value().toString());
                Airport.addLongitude(builder, longitude);
            }
            if (properties.containsKey(PROPERTY_ALTITUDE))
            {
                int altitude = Integer.parseInt(properties.get(PROPERTY_ALTITUDE).get(0).value().toString());
                Airport.addAltitude(builder, altitude);
            }
            if (properties.containsKey(PROPERTY_TIMEZONE))
            {
                float timezone = Float.parseFloat(properties.get(PROPERTY_TIMEZONE).get(0).value().toString());
                Airport.addTimezone(builder, timezone);
            }
            if (properties.containsKey(PROPERTY_DST))
            {
                Airport.addDst(builder, dstOffset);
            }
            if (properties.containsKey(PROPERTY_TZTEXT))
            {
                Airport.addTztext(builder, tztextOffset);
            }
            if (properties.containsKey(PROPERTY_TYPE))
            {
                Airport.addType(builder, typeOffset);
            }
            if (properties.containsKey(PROPERTY_SOURCE))
            {
                Airport.addSource(builder, sourceOffset);
            }

            int airportOffset = Airport.endAirport(builder);
            builder.finish(airportOffset);
        }
        else
        {
            return null;
        }

        return builder.sizedByteArray();
    }

    private static byte[] packAirportEdgeProperties(long type, Map<String, Property> properties)
    {
        if (properties == null)
        {
            return null;
        }

        FlatBufferBuilder builder = new FlatBufferBuilder();

        int airlineOffset = 0;
        int srcApOffset = 0;
        int dstApOffset = 0;
        int codeshareOffset = 0;
        int equipmentOffset = 0;

        if (type == ROUTE_EDGE_TYPE)
        {
            // Serialize string fields before serializing the entire object.
            if (properties.containsKey(PROPERTY_AIRLINE))
            {
                airlineOffset = builder.createString(properties.get(PROPERTY_AIRLINE).value().toString());
            }
            if (properties.containsKey(PROPERTY_SRC_AP))
            {
                srcApOffset = builder.createString(properties.get(PROPERTY_SRC_AP).value().toString());
            }
            if (properties.containsKey(PROPERTY_DST_AP))
            {
                dstApOffset = builder.createString(properties.get(PROPERTY_DST_AP).value().toString());
            }
            if (properties.containsKey(PROPERTY_CODESHARE))
            {
                codeshareOffset = builder.createString(properties.get(PROPERTY_CODESHARE).value().toString());
            }
            if (properties.containsKey(PROPERTY_EQUIPMENT))
            {
                equipmentOffset = builder.createString(properties.get(PROPERTY_EQUIPMENT).value().toString());
            }

            Route.startRoute(builder);

            if (properties.containsKey(PROPERTY_AIRLINE))
            {
                Route.addAirline(builder, airlineOffset);
            }
            if (properties.containsKey(PROPERTY_AL_ID))
            {
                int alId = Integer.parseInt(properties.get(PROPERTY_AL_ID).value().toString());
                Route.addAlId(builder, alId);
            }
            if (properties.containsKey(PROPERTY_SRC_AP))
            {
                Route.addSrcAp(builder, srcApOffset);
            }
            if (properties.containsKey(PROPERTY_SRC_AP_ID))
            {
                int srcApId = Integer.parseInt(properties.get(PROPERTY_SRC_AP_ID).value().toString());
                Route.addSrcApId(builder, srcApId);
            }
            if (properties.containsKey(PROPERTY_DST_AP))
            {
                Route.addSrcAp(builder, dstApOffset);
            }
            if (properties.containsKey(PROPERTY_DST_AP_ID))
            {
                int dstApId = Integer.parseInt(properties.get(PROPERTY_DST_AP_ID).value().toString());
                Route.addSrcApId(builder, dstApId);
            }
            if (properties.containsKey(PROPERTY_CODESHARE))
            {
                Route.addCodeshare(builder, codeshareOffset);
            }
            if (properties.containsKey(PROPERTY_STOPS))
            {
                int stops = Integer.parseInt(properties.get(PROPERTY_STOPS).value().toString());
                Route.addStops(builder, stops);
            }
            if (properties.containsKey(PROPERTY_EQUIPMENT))
            {
                Route.addEquipment(builder, equipmentOffset);
            }

            int routeOffset = Route.endRoute(builder);
            builder.finish(routeOffset);
        }
        else
        {
            return null;
        }

        return builder.sizedByteArray();
    }

    protected static boolean createAirportNode(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        long type = getTypeForLabel(vertex.label);
        byte[] payload = packAirportNodeProperties(type, vertex.properties);

        boolean ownTransaction = startTransaction(graph);
        long idNode = graph.gaiaDb.createNode(id, type, payload);
        return handleTransaction(graph, idNode != 0, ownTransaction);
    }

    protected static boolean updateAirportNodePayload(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        long type = getTypeForLabel(vertex.label);
        byte[] payload = packAirportNodeProperties(type, vertex.properties);

        boolean ownTransaction = startTransaction(graph);
        return handleTransaction(graph, graph.gaiaDb.updateNodePayload(id, payload), ownTransaction);
    }

    protected static boolean createAirportEdge(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        long type = getTypeForLabel(edge.label);
        byte[] payload = packAirportEdgeProperties(type, edge.properties);
        long idFirstNode = Long.parseLong(edge.outVertex.id.toString());
        long idSecondNode = Long.parseLong(edge.inVertex.id.toString());

        boolean ownTransaction = startTransaction(graph);
        long idEdge = graph.gaiaDb.createEdge(id, type, idFirstNode, idSecondNode, payload);
        return handleTransaction(graph, idEdge != 0, ownTransaction);
    }

    protected static boolean updateAirportEdgePayload(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        long type = getTypeForLabel(edge.label);
        byte[] payload = packAirportEdgeProperties(type, edge.properties);

        boolean ownTransaction = startTransaction(graph);
        return handleTransaction(graph, graph.gaiaDb.updateEdgePayload(id, payload), ownTransaction);
    }

    // In-memory graph helpers.
    protected static Edge addEdge(
        final CacheGraph graph,
        final CacheVertex outVertex, final CacheVertex inVertex,
        final String label,
        final Object... keyValues)
    {
        ElementHelper.validateLabel(label);
        ElementHelper.legalPropertyKeyValueArray(keyValues);

        Object idValue = graph.edgeIdManager.convert(ElementHelper.getIdValue(keyValues).orElse(null));
        if (idValue != null)
        {
            if (graph.edges.containsKey(idValue))
            {
                throw Graph.Exceptions.edgeWithIdAlreadyExists(idValue);
            }
        }
        else
        {
            idValue = graph.edgeIdManager.getNextId(graph);
        }

        final Edge edge = new CacheEdge(idValue, label, outVertex, inVertex);
        ElementHelper.attachProperties(edge, keyValues);

        // Create edge in Gaia database.
        if (!createEdge((CacheEdge)edge))
        {
            throw new UnsupportedOperationException("Gaia database edge creation failed!");
        }

        graph.edges.put(edge.id(), edge);
        CacheHelper.addOutEdge(outVertex, label, edge);
        CacheHelper.addInEdge(inVertex, label, edge);

        return edge;
    }

    protected static void addOutEdge(
        final CacheVertex vertex, final String label, final Edge edge)
    {
        if (vertex.outEdges == null)
        {
            vertex.outEdges = new ConcurrentHashMap<>();
        }

        Set<Edge> edges = vertex.outEdges.get(label);
        if (edges == null)
        {
            edges = new HashSet<>();
            vertex.outEdges.put(label, edges);
        }

        edges.add(edge);
    }

    protected static void addInEdge(
        final CacheVertex vertex, final String label, final Edge edge)
    {
        if (vertex.inEdges == null)
        {
            vertex.inEdges = new ConcurrentHashMap<>();
        }

        Set<Edge> edges = vertex.inEdges.get(label);
        if (edges == null)
        {
            edges = new HashSet<>();
            vertex.inEdges.put(label, edges);
        }

        edges.add(edge);
    }

    protected static Iterator<CacheVertex> getVertices(
        final CacheVertex vertex, final Direction direction, final String... edgeLabels)
    {
        final List<Vertex> vertices = new ArrayList<>();

        if (direction.equals(Direction.OUT) || direction.equals(Direction.BOTH))
        {
            if (vertex.outEdges != null)
            {
                if (edgeLabels.length == 0)
                {
                    vertex.outEdges.values().forEach(set -> set
                        .forEach(edge -> vertices.add(((CacheEdge)edge).inVertex)));
                }
                else if (edgeLabels.length == 1)
                {
                    vertex.outEdges.getOrDefault(edgeLabels[0], Collections.emptySet())
                        .forEach(edge -> vertices.add(((CacheEdge)edge).inVertex));
                }
                else
                {
                    Stream.of(edgeLabels).map(vertex.outEdges::get)
                        .filter(Objects::nonNull)
                        .flatMap(Set::stream)
                        .forEach(edge -> vertices.add(((CacheEdge)edge).inVertex));
                }
            }
        }

        if (direction.equals(Direction.IN) || direction.equals(Direction.BOTH))
        {
            if (vertex.inEdges != null)
            {
                if (edgeLabels.length == 0)
                {
                    vertex.inEdges.values().forEach(set -> set
                        .forEach(edge -> vertices.add(((CacheEdge)edge).outVertex)));
                }
                else if (edgeLabels.length == 1)
                {
                    vertex.inEdges.getOrDefault(edgeLabels[0], Collections.emptySet())
                        .forEach(edge -> vertices.add(((CacheEdge)edge).outVertex));
                }
                else
                {
                    Stream.of(edgeLabels).map(vertex.inEdges::get)
                        .filter(Objects::nonNull)
                        .flatMap(Set::stream)
                        .forEach(edge -> vertices.add(((CacheEdge)edge).outVertex));
                }
            }
        }

        return (Iterator) vertices.iterator();
    }

    protected static Iterator<CacheEdge> getEdges(
        final CacheVertex vertex, final Direction direction, final String... edgeLabels)
    {
        final List<Edge> edges = new ArrayList<>();

        if (direction.equals(Direction.OUT) || direction.equals(Direction.BOTH))
        {
            if (vertex.outEdges != null)
            {
                if (edgeLabels.length == 0)
                {
                    vertex.outEdges.values().forEach(edges::addAll);
                }
                else if (edgeLabels.length == 1)
                {
                    edges.addAll(vertex.outEdges.getOrDefault(edgeLabels[0], Collections.emptySet()));
                }
                else
                {
                    Stream.of(edgeLabels).map(vertex.outEdges::get).filter(Objects::nonNull).forEach(edges::addAll);
                }
            }
        }

        if (direction.equals(Direction.IN) || direction.equals(Direction.BOTH))
        {
            if (vertex.inEdges != null)
            {
                if (edgeLabels.length == 0)
                {
                    vertex.inEdges.values().forEach(edges::addAll);
                }
                else if (edgeLabels.length == 1)
                {
                    edges.addAll(vertex.inEdges.getOrDefault(edgeLabels[0], Collections.emptySet()));
                }
                else
                {
                    Stream.of(edgeLabels).map(vertex.inEdges::get).filter(Objects::nonNull).forEach(edges::addAll);
                }
            }
        }

        return (Iterator) edges.iterator();
    }
}
