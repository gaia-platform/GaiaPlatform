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

public final class CacheHelper
{
    private final static String propertyDelimiter = "|";
    private final static String keyValueDelimiter = "=";
    private final static String emptyString = "";

    private static AtomicLong lastType = new AtomicLong();
    private static Map<String, Long> mapLabelsToTypes = new ConcurrentHashMap<>();

    private CacheHelper()
    {
    }

    private static void packProperty(StringBuilder payload, String key, String value)
    {
        if (payload.length() > 0)
        {
            payload.append(propertyDelimiter);
        }

        payload.append(key);
        payload.append(keyValueDelimiter);
        payload.append(value);
    }

    private static String packProperties(Map<String, Property> properties)
    {
        if (properties == null)
        {
            return emptyString;
        }

        StringBuilder payload = new StringBuilder();

        properties.forEach((key, property) -> packProperty(payload, key, property.value().toString()));

        return payload.toString();
    }

    private static String packPropertyLists(Map<String, List<VertexProperty>> properties)
    {
        if (properties == null)
        {
            return emptyString;
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
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        long type = getTypeForLabel(vertex.label);
        String payload = packPropertyLists(vertex.properties);

        graph.cow.beginTransaction();
        long idNode = graph.cow.createNode(id, type, payload);
        return handleTransaction(graph, idNode != 0);
    }

    protected static boolean removeNode(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.removeNode(id));
    }

    protected static boolean updateNodePayload(CacheVertex vertex)
    {
        CacheGraph graph = vertex.graph;
        long id = Long.parseLong(vertex.id.toString());
        String payload = packPropertyLists(vertex.properties);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.updateNodePayload(id, payload));
    }

    protected static boolean createEdge(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        long type = getTypeForLabel(edge.label);
        String payload = packProperties(edge.properties);
        long idFirstNode = Long.parseLong(edge.outVertex.id.toString());
        long idSecondNode = Long.parseLong(edge.inVertex.id.toString());

        graph.cow.beginTransaction();
        long idEdge = graph.cow.createEdge(id, type, idFirstNode, idSecondNode, payload);
        return handleTransaction(graph, idEdge != 0);
    }

    protected static boolean removeEdge(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.removeEdge(id));
    }

    protected static boolean updateEdgePayload(CacheEdge edge)
    {
        CacheGraph graph = edge.graph;
        long id = Long.parseLong(edge.id.toString());
        String payload = packProperties(edge.properties);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.updateEdgePayload(id, payload));
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
