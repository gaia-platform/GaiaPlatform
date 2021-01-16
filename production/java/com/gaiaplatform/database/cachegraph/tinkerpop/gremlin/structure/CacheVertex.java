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
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class CacheVertex extends CacheElement implements Vertex
{
    // outEdges are the outgoing edges.
    protected Map<String, Set<Edge>> outEdges;
    // inEdges are the incoming edges.
    protected Map<String, Set<Edge>> inEdges;

    protected Map<String, List<VertexProperty>> properties;

    protected CacheVertex(final Graph graph, final Object id, final String label)
    {
        super(graph, id, label);

        this.supportsNullPropertyValues = this.graph.supportsNullPropertyValues;
    }

    public Edge addEdge(final String label, final Vertex inVertex, final Object... keyValues)
    {
        CacheHelper.debugPrint(this.graph, "vertex::addEdge()");

        if (this.removed)
        {
            throw elementAlreadyRemoved(Vertex.class, this.id);
        }

        if (inVertex == null)
        {
            throw Graph.Exceptions.argumentCanNotBeNull("inVertex");
        }

        return CacheHelper.addEdge(this.graph, this, (CacheVertex)inVertex, label, keyValues);
    }

    public Set<String> keys()
    {
        return (this.properties == null) ? Collections.emptySet() : this.properties.keySet();
    }

    public <V> VertexProperty<V> property(
        final VertexProperty.Cardinality cardinality,
        final String key,
        final V value,
        final Object... keyValues)
    {
        CacheHelper.debugPrint(this.graph, "vertex::property()");

        if (this.removed)
        {
            throw elementAlreadyRemoved(Vertex.class, id);
        }

        ElementHelper.validateProperty(key, value);
        ElementHelper.legalPropertyKeyValueArray(keyValues);

        // If we don't support null property values and the value is null,
        // then the key can be removed, but only if the cardinality is single.
        // If it is list/set, then we can just ignore the null.
        if (!this.supportsNullPropertyValues && value == null)
        {
            if (this.properties != null)
            {
                final VertexProperty.Cardinality propertyCardinality = (cardinality == null)
                    ? this.graph.features().vertex().getCardinality(key) : cardinality;

                if (propertyCardinality == VertexProperty.Cardinality.single)
                {
                    this.properties(key).forEachRemaining(VertexProperty::remove);
                }
            }

            return VertexProperty.empty();
        }

        final Optional<Object> optionalId = ElementHelper.getIdValue(keyValues);
        final Optional<VertexProperty<V>> optionalVertexProperty = ElementHelper.stageVertexProperty(
            this, cardinality, key, value, keyValues);

        if (optionalVertexProperty.isPresent())
        {
            return optionalVertexProperty.get();
        }

        final Object idValue = optionalId.isPresent()
            ? this.graph.vertexPropertyIdManager.convert(optionalId.get())
            : this.graph.vertexPropertyIdManager.getNextId(graph);

        final VertexProperty<V> newVertexProperty = new CacheVertexProperty<V>(idValue, this, key, value);

        if (this.properties == null)
        {
            this.properties = new ConcurrentHashMap<>();
        }

        final List<VertexProperty> list = this.properties.getOrDefault(key, new ArrayList<>());
        list.add(newVertexProperty);
        this.properties.put(key, list);

        ElementHelper.attachProperties(newVertexProperty, keyValues);

        // Update node payload in Gaia database.
        // We don't have to do this if the node is being created,
        // which we detect by checking if it's been added to the graph.vertices map.
        // No plans to support vertex property properties in Gaia database for now.
        if (this.graph.vertices.containsKey(id) && !CacheHelper.updateNodePayload(this))
        {
            throw new UnsupportedOperationException("Gaia database node update failed!");
        }

        return newVertexProperty;
    }

    public <V> Iterator<VertexProperty<V>> properties(final String... propertyKeys)
    {
        if (this.removed || this.properties == null)
        {
            return Collections.emptyIterator();
        }

        if (propertyKeys.length == 1)
        {
            final List<VertexProperty> properties = this.properties.getOrDefault(
                propertyKeys[0], Collections.emptyList());

            if (properties.size() == 1)
            {
                return IteratorUtils.of(properties.get(0));
            }
            else if (properties.isEmpty())
            {
                return Collections.emptyIterator();
            }
            else
            {
                return (Iterator)new ArrayList<>(properties).iterator();
            }
        }
        else
        {
            return (Iterator)(this.properties.entrySet().stream()
                .filter(entry -> ElementHelper.keyExists(entry.getKey(), propertyKeys))
                .flatMap(entry -> entry.getValue().stream())
                .collect(Collectors.toList()).iterator());
        }
    }

    public Iterator<Vertex> vertices(final Direction direction, final String... edgeLabels)
    {
        return (Iterator)CacheHelper.getVertices(this, direction, edgeLabels);
    }

    public Iterator<Edge> edges(final Direction direction, final String... edgeLabels)
    {
        return (Iterator)CacheHelper.getEdges(this, direction, edgeLabels);
    }

    public void remove()
    {
        CacheHelper.debugPrint(this.graph, "vertex::remove()");

        // First remove all edges related to this node.
        // This will also remove the edges from Gaia database.
        final List<Edge> edges = new ArrayList<>();
        this.edges(Direction.BOTH).forEachRemaining(edges::add);
        edges.stream().filter(edge -> !((CacheEdge)edge).removed).forEach(Edge::remove);

        // Remove the node from Gaia database.
        if (!CacheHelper.removeNode(this))
        {
            throw new UnsupportedOperationException("Gaia database node deletion failed!");
        }

        // Then remove the node.
        this.graph.vertices.remove(this.id);

        this.properties = null;
        this.removed = true;
    }

    public String toString()
    {
        return StringFactory.vertexString(this);
    }
}
