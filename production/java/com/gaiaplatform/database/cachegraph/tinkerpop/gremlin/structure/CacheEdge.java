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

import java.util.Collections;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class CacheEdge extends CacheElement implements Edge
{
    // Our edge goes from outVertex to inVertex.
    protected final CacheVertex outVertex;
    protected final CacheVertex inVertex;

    protected Map<String, Property> properties;

    protected CacheEdge(
        final Object id,
        final String label,
        final Vertex outVertex, final Vertex inVertex)
    {
        super(inVertex.graph(), id, label);

        this.outVertex = (CacheVertex)outVertex;
        this.inVertex = (CacheVertex)inVertex;
    }

    public Vertex outVertex()
    {
        return this.outVertex;
    }

    public Vertex inVertex()
    {
        return this.inVertex;
    }

    public Iterator<Vertex> vertices(final Direction direction)
    {
        if (removed)
        {
            return Collections.emptyIterator();
        }
        
        switch (direction)
        {
            case OUT:
                return IteratorUtils.of(this.outVertex);

            case IN:
                return IteratorUtils.of(this.inVertex);

            default:
                return IteratorUtils.of(this.outVertex, this.inVertex);
        }
    }

    public Set<String> keys()
    {
        return (this.properties == null) ? Collections.emptySet() : this.properties.keySet();
    }

    public <V> Property<V> property(final String key, final V value)
    {
        if (this.removed)
        {
            throw elementAlreadyRemoved(Edge.class, this.id);
        }

        ElementHelper.validateProperty(key, value);

        final Property<V> newProperty = new CacheProperty<>(this, key, value);

        if (this.properties == null)
        {
            this.properties = new ConcurrentHashMap<>();
        }

        this.properties.put(key, newProperty);

        // Update edge payload in COW.
        // We don't have to do this if the edge is being created,
        // which we detect by checking if it's been added to the graph.edges map.
        if (this.graph.edges.containsKey(id) && !CacheHelper.updateEdgePayload(this))
        {
            throw new UnsupportedOperationException("COW edge update failed!");
        }

        return newProperty;
    }

    public <V> Property<V> property(final String key)
    {
        return (this.properties == null)
            ? Property.<V>empty()
            : this.properties.getOrDefault(key, Property.<V>empty());
    }

    public <V> Iterator<Property<V>> properties(final String... propertyKeys)
    {
        if (this.properties == null)
        {
            return Collections.emptyIterator();
        }

        if (propertyKeys.length == 1)
        {
            final Property<V> property = this.properties.get(propertyKeys[0]);
            return (property == null) ? Collections.emptyIterator() : IteratorUtils.of(property);
        }
        else
        {
            return (Iterator) this.properties.entrySet().stream()
                .filter(entry -> ElementHelper.keyExists(entry.getKey(), propertyKeys))
                .map(entry -> entry.getValue()).collect(Collectors.toList()).iterator();
        }
    }

    public void remove()
    {
        // Remove the edge from COW.
        if (!CacheHelper.removeEdge(this))
        {
            throw new UnsupportedOperationException("COW edge deletion failed!");
        }

        final CacheVertex outVertex = (CacheVertex)this.outVertex;
        if (outVertex != null && outVertex.outEdges != null)
        {
            final Set<Edge> edges = outVertex.outEdges.get(this.label);
            if (edges != null)
            {
                edges.remove(this);
            }
        }

        final CacheVertex inVertex = (CacheVertex)this.inVertex;
        if (inVertex != null && inVertex.inEdges != null)
        {
            final Set<Edge> edges = inVertex.inEdges.get(this.label);
            if (edges != null)
            {
                edges.remove(this);
            }
        }

        this.graph.edges.remove(this.id);

        this.properties = null;
        this.removed = true;
    }

    public String toString()
    {
        return StringFactory.edgeString(this);
    }
}
