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
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

import com.google.flatbuffers.FlatBufferBuilder;

import com.gaiaplatform.database.CowStorageEngine;
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

    protected static void loadAirportGraphFromCow(CacheGraph graph)
    {
        graph.cow.beginTransaction();

        // Scan airline nodes.
        long currentNodeId = graph.cow.findFirstNode(AIRLINE_NODE_TYPE);
        while (currentNodeId != 0)
        {
            graph.cow.printNode(currentNodeId);
            currentNodeId = graph.cow.findNextNode(currentNodeId);
        }

        // Scan airport nodes.
        currentNodeId = graph.cow.findFirstNode(AIRPORT_NODE_TYPE);
        while (currentNodeId != 0)
        {
            graph.cow.printNode(currentNodeId);
            currentNodeId = graph.cow.findNextNode(currentNodeId);
        }

        // Scan route edges.
        long currentEdgeId = graph.cow.findFirstEdge(ROUTE_EDGE_TYPE);
        while (currentEdgeId != 0)
        {
            graph.cow.printEdge(currentEdgeId);
            currentEdgeId = graph.cow.findNextEdge(currentEdgeId);
        }

        // We're not making any changes to COW, so we can just rollback.
        graph.cow.rollbackTransaction();
    }

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

    private static boolean handleTransaction(CacheGraph graph, boolean operationResult)
    {
        if (operationResult)
        {
            graph.cow.commitTransaction();
        }
        else
        {
            graph.cow.rollbackTransaction();
        }

        return operationResult;
    }

    protected static boolean createNode(CacheVertex vertex)
    {
        if (!vertex.graph.enableCowWrites)
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
        if (!vertex.graph.enableCowWrites)
        {
            return true;
        }

        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.removeNode(id));
    }

    protected static boolean updateNodePayload(CacheVertex vertex)
    {
        if (!vertex.graph.enableCowWrites)
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
        if (!edge.graph.enableCowWrites)
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
        if (!edge.graph.enableCowWrites)
        {
            return true;
        }

        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.removeEdge(id));
    }

    protected static boolean updateEdgePayload(CacheEdge edge)
    {
        if (!edge.graph.enableCowWrites)
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

    protected static boolean createGenericNode(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        long type = getTypeForLabel(vertex.label);
        String payload = packNodeProperties(vertex.properties);

        graph.cow.beginTransaction();
        long idNode = graph.cow.createNode(id, type, payload);
        return handleTransaction(graph, idNode != 0);
    }

    protected static boolean updateGenericNodePayload(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        String payload = packNodeProperties(vertex.properties);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.updateNodePayload(id, payload));
    }

    protected static boolean createGenericEdge(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        long type = getTypeForLabel(edge.label);
        String payload = packEdgeProperties(edge.properties);
        long idFirstNode = Long.parseLong(edge.outVertex.id.toString());
        long idSecondNode = Long.parseLong(edge.inVertex.id.toString());

        graph.cow.beginTransaction();
        long idEdge = graph.cow.createEdge(id, type, idFirstNode, idSecondNode, payload);
        return handleTransaction(graph, idEdge != 0);
    }

    protected static boolean updateGenericEdgePayload(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        String payload = packEdgeProperties(edge.properties);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.updateEdgePayload(id, payload));
    }

    private static byte[] packAirportEdgeProperties(long type, Map<String, Property> properties)
    {
        if (properties == null)
        {
            return null;
        }

        FlatBufferBuilder builder = new FlatBufferBuilder();

        if (type == ROUTE_EDGE_TYPE)
        {
            Route.startRoute(builder);
            if (properties.containsKey(PROPERTY_AIRLINE))
            {
                int airlineOffset = builder.createString(properties.get(PROPERTY_AIRLINE).value().toString());
                Route.addAirline(builder, airlineOffset);
            }
            if (properties.containsKey(PROPERTY_AL_ID))
            {
                int alId = Integer.parseInt(properties.get(PROPERTY_AL_ID).value().toString());
                Route.addAlId(builder, alId);
            }
            if (properties.containsKey(PROPERTY_SRC_AP))
            {
                int srcApOffset = builder.createString(properties.get(PROPERTY_SRC_AP).value().toString());
                Route.addSrcAp(builder, srcApOffset);
            }
            if (properties.containsKey(PROPERTY_SRC_AP_ID))
            {
                int srcApId = Integer.parseInt(properties.get(PROPERTY_SRC_AP_ID).value().toString());
                Route.addSrcApId(builder, srcApId);
            }
            if (properties.containsKey(PROPERTY_DST_AP))
            {
                int dstApOffset = builder.createString(properties.get(PROPERTY_DST_AP).value().toString());
                Route.addSrcAp(builder, dstApOffset);
            }
            if (properties.containsKey(PROPERTY_DST_AP_ID))
            {
                int dstApId = Integer.parseInt(properties.get(PROPERTY_DST_AP_ID).value().toString());
                Route.addSrcApId(builder, dstApId);
            }
            if (properties.containsKey(PROPERTY_CODESHARE))
            {
                int codeshareOffset = builder.createString(properties.get(PROPERTY_CODESHARE).value().toString());
                Route.addCodeshare(builder, codeshareOffset);
            }
            if (properties.containsKey(PROPERTY_STOPS))
            {
                int stops = Integer.parseInt(properties.get(PROPERTY_STOPS).value().toString());
                Route.addStops(builder, stops);
            }
            if (properties.containsKey(PROPERTY_EQUIPMENT))
            {
                int equipmentOffset = builder.createString(properties.get(PROPERTY_EQUIPMENT).value().toString());
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

    private static byte[] packAirportNodeProperties(long type, Map<String, List<VertexProperty>> properties)
    {
        if (properties == null)
        {
            return null;
        }

        FlatBufferBuilder builder = new FlatBufferBuilder();

        if (type == AIRLINE_NODE_TYPE)
        {
            Airline.startAirline(builder);

            int airlineOffset = Airline.endAirline(builder);
            builder.finish(airlineOffset);
        }
        else if (type == AIRPORT_NODE_TYPE)
        {
            Airport.startAirport(builder);

            int airportOffset = Airport.endAirport(builder);
            builder.finish(airportOffset);
        }
        else
        {
            return null;
        }
    
        return builder.sizedByteArray();
    }

    protected static boolean createAirportNode(CacheVertex vertex)
    {
        return false;
    }

    protected static boolean updateAirportNodePayload(CacheVertex vertex)
    {
        return false;
    }

    protected static boolean createAirportEdge(CacheEdge edge)
    {
        return false;
    }

    protected static boolean updateAirportEdgePayload(CacheEdge edge)
    {
        return false;
    }

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

        // Create edge in COW.
        if (!createEdge((CacheEdge)edge))
        {
            throw new UnsupportedOperationException("COW edge creation failed!");
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
