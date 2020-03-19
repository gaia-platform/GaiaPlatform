/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class TrueGraphDBVertex extends TrueGraphDBElement implements Vertex
{
    protected Map<String, List<VertexProperty>> properties;

    protected TrueGraphDBVertex(final Graph graph, final Object id, final String label)
    {
        super(graph, id, label);
    }

    public Edge addEdge(final String label, final Vertex inVertex, final Object... keyValues)
    {
        if (inVertex == null)
        {
            throw Graph.Exceptions.argumentCanNotBeNull("inVertex");
        }

        ElementHelper.validateLabel(label);
		ElementHelper.legalPropertyKeyValueArray(keyValues);
        
        // TODO: Add implementation and create edge in COW.

        return null;
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
        ElementHelper.validateProperty(key, value);
        ElementHelper.legalPropertyKeyValueArray(keyValues);

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

        final VertexProperty<V> newVertexProperty = new TrueGraphDBVertexProperty<V>(idValue, this, key, value);

        if (this.properties == null)
        {
            this.properties = new HashMap<>();
        } 

        final List<VertexProperty> list = this.properties.getOrDefault(key, new ArrayList<>());
        list.add(newVertexProperty);
        this.properties.put(key, list);

        ElementHelper.attachProperties(newVertexProperty, keyValues);

        // TODO: Update node payload in COW.

        return newVertexProperty;
    }

    public Iterator<Vertex> vertices(final Direction direction, final String... edgeLabels)
    {
        // TODO: Add implementation.

        return Collections.emptyIterator();
    }

    public Iterator<Edge> edges(final Direction direction, final String... edgeLabels)
    {
        // TODO: Add implementation.

        return Collections.emptyIterator();
    }

    public <V> Iterator<VertexProperty<V>> properties(final String... propertyKeys)
    {
        if (removed || this.properties == null)
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

    public void remove()
    {
        // First remove all edges related to this node.
        // This will also remove the edges from COW.
        final List<Edge> edges = new ArrayList<>();
        this.edges(Direction.BOTH).forEachRemaining(edges::add);
        edges.stream().filter(edge -> !((TrueGraphDBEdge)edge).removed).forEach(Edge::remove);

        // TODO: Remove the node from COW.

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
