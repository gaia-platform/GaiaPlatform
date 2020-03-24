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

import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.apache.tinkerpop.gremlin.structure.*;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class TrueGraphDBHelper
{
    protected final static String propertyDelimiter = "|";
    protected final static String keyValueDelimiter = "=";

    private TrueGraphDBHelper()
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
        StringBuilder payload = new StringBuilder();

        properties.forEach((key, property) -> packProperty(payload, key, property.value().toString()));

        return payload.toString();
    }

    private static String packPropertyLists(Map<String, List<VertexProperty>> properties)
    {
        StringBuilder payload = new StringBuilder();

        properties.forEach((key, list) -> packProperty(payload, key, list.get(0).value().toString()));

        return payload.toString();
    }

    private static boolean handleTransaction(TrueGraphDBGraph graph, boolean operationResult)
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

    protected static boolean createNode(TrueGraphDBVertex vertex)
    {
        TrueGraphDBGraph graph = vertex.graph;
        long id = Long.parseLong((String)vertex.id);
        long type = Long.parseLong((String)vertex.label);
        String payload = packPropertyLists(vertex.properties);

        graph.cow.beginTransaction();
        long idNode = graph.cow.createNode(id, type, payload);
        return handleTransaction(graph, idNode != 0);
    }

    protected static boolean removeNode(TrueGraphDBVertex vertex)
    {
        TrueGraphDBGraph graph = vertex.graph;
        long id = Long.parseLong((String)vertex.id);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.removeNode(id));
    }

    protected static boolean updateNodePayload(TrueGraphDBVertex vertex)
    {
        TrueGraphDBGraph graph = vertex.graph;
        long id = Long.parseLong((String)vertex.id);
        String payload = packPropertyLists(vertex.properties);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.updateNodePayload(id, payload));
    }

    protected static boolean createEdge(TrueGraphDBEdge edge)
    {
        TrueGraphDBGraph graph = edge.graph;
        long id = Long.parseLong((String)edge.id);
        long type = Long.parseLong((String)edge.label);
        String payload = packProperties(edge.properties);
        long idIn = Long.parseLong((String)edge.inVertex.id);
        long idOut = Long.parseLong((String)edge.outVertex.id);

        graph.cow.beginTransaction();
        long idEdge = graph.cow.createEdge(id, type, idIn, idOut, payload);
        return handleTransaction(graph, idEdge != 0);
    }

    protected static boolean removeEdge(TrueGraphDBEdge edge)
    {
        TrueGraphDBGraph graph = edge.graph;
        long id = Long.parseLong((String)edge.id);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.removeEdge(id));
    }

    protected static boolean updateEdgePayload(TrueGraphDBEdge edge)
    {
        TrueGraphDBGraph graph = edge.graph;
        long id = Long.parseLong((String)edge.id);
        String payload = packProperties(edge.properties);

        graph.cow.beginTransaction();
        return handleTransaction(graph, graph.cow.updateEdgePayload(id, payload));
    }

    protected static Edge addEdge(
        final TrueGraphDBGraph graph,
        final TrueGraphDBVertex inVertex, final TrueGraphDBVertex outVertex,
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

        final Edge edge = new TrueGraphDBEdge(idValue, label, inVertex, outVertex);
        ElementHelper.attachProperties(edge, keyValues);

        // Create edge in COW.
        if (!createEdge((TrueGraphDBEdge)edge))
        {
            throw new UnsupportedOperationException("COW edge creation failed!");
        }

        graph.edges.put(edge.id(), edge);
        TrueGraphDBHelper.addInEdge(inVertex, label, edge);
        TrueGraphDBHelper.addOutEdge(outVertex, label, edge);

        return edge;
    }

    protected static void addInEdge(
        final TrueGraphDBVertex vertex, final String label, final Edge edge)
    {
        if (vertex.inEdges == null)
        {
            vertex.inEdges = new HashMap<>();   
        }

        Set<Edge> edges = vertex.inEdges.get(label);
        if (edges == null)
        {
            edges = new HashSet<>();
            vertex.inEdges.put(label, edges);
        }

        edges.add(edge);
    }

    protected static void addOutEdge(
        final TrueGraphDBVertex vertex, final String label, final Edge edge)
    {
        if (vertex.outEdges == null)
        {
            vertex.outEdges = new HashMap<>();
        }

        Set<Edge> edges = vertex.outEdges.get(label);
        if (edges == null)
        {
            edges = new HashSet<>();
            vertex.outEdges.put(label, edges);
        }

        edges.add(edge);
    }

    protected static Iterator<TrueGraphDBVertex> getVertices(
        final TrueGraphDBVertex vertex, final Direction direction, final String... edgeLabels)
    {
        final List<Vertex> vertices = new ArrayList<>();

        if (direction.equals(Direction.OUT) || direction.equals(Direction.BOTH))
        {
            if (vertex.outEdges != null)
            {
                if (edgeLabels.length == 0)
                {
                    vertex.outEdges.values().forEach(set -> set
                        .forEach(edge -> vertices.add(((TrueGraphDBEdge)edge).inVertex)));
                }
                else if (edgeLabels.length == 1)
                {
                    vertex.outEdges.getOrDefault(edgeLabels[0], Collections.emptySet())
                        .forEach(edge -> vertices.add(((TrueGraphDBEdge)edge).inVertex));
                }
                else
                {
                    Stream.of(edgeLabels).map(vertex.outEdges::get)
                        .filter(Objects::nonNull)
                        .flatMap(Set::stream)
                        .forEach(edge -> vertices.add(((TrueGraphDBEdge)edge).inVertex));
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
                        .forEach(edge -> vertices.add(((TrueGraphDBEdge)edge).outVertex)));
                }
                else if (edgeLabels.length == 1)
                {
                    vertex.inEdges.getOrDefault(edgeLabels[0], Collections.emptySet())
                        .forEach(edge -> vertices.add(((TrueGraphDBEdge)edge).outVertex));
                }
                else
                {
                    Stream.of(edgeLabels).map(vertex.inEdges::get)
                        .filter(Objects::nonNull)
                        .flatMap(Set::stream)
                        .forEach(edge -> vertices.add(((TrueGraphDBEdge)edge).outVertex));
                }
            }
        }

        return (Iterator) vertices.iterator();
    }

    protected static Iterator<TrueGraphDBEdge> getEdges(
        final TrueGraphDBVertex vertex, final Direction direction, final String... edgeLabels)
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
