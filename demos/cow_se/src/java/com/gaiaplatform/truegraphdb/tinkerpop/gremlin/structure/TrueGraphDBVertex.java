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

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;

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

        TrueGraphDBGraph graph = (TrueGraphDBGraph)graph();
        final Object idValue = optionalId.isPresent()
            ? graph.vertexPropertyIdManager.convert(optionalId.get())
            : graph.vertexPropertyIdManager.getNextId(graph);

        final VertexProperty<V> newVertexProperty = new TrueGraphDBVertexProperty<V>(this, key, value, idValue);

        if (this.properties == null)
        {
            this.properties = new HashMap<>();
        } 

        final List<VertexProperty> list = this.properties.getOrDefault(key, new ArrayList<>());
        list.add(newVertexProperty);
        this.properties.put(key, list);

        ElementHelper.attachProperties(newVertexProperty, keyValues);

        return newVertexProperty;
    }

    public Iterator<Edge> edges(final Direction direction, final String... edgeLabels)
    {
        return Collections.emptyIterator();
    }

    public Iterator<Vertex> vertices(final Direction direction, final String... edgeLabels)
    {
        return Collections.emptyIterator();
    }

    public <V> Iterator<VertexProperty<V>> properties(final String... propertyKeys)
    {
        if (removed)
        {
            return Collections.emptyIterator();
        }

        return Collections.emptyIterator();
    }

    public void remove()
    {
        this.removed = true;
    }

    public String toString()
    {
        return StringFactory.vertexString(this);
    }
}
