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

import org.apache.tinkerpop.gremlin.structure.*;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class TrueGraphDBEdge extends TrueGraphDBElement implements Edge
{
    protected final TrueGraphDBVertex inVertex;
    protected final TrueGraphDBVertex outVertex;

    protected Map<String, Property> properties;

    protected TrueGraphDBEdge(
        final Object id,
        final String label,
        final Vertex inVertex, final Vertex outVertex)
    {
        super(inVertex.graph(), id, label);

        this.inVertex = (TrueGraphDBVertex)inVertex;
        this.outVertex = (TrueGraphDBVertex)outVertex;
    }

    public Vertex inVertex()
    {
        return this.inVertex;
    }

    public Vertex outVertex()
    {
        return this.outVertex;
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

        final Property<V> newProperty = new TrueGraphDBProperty<>(this, key, value);

        if (this.properties == null)
        {
            this.properties = new HashMap<>();
        }

        this.properties.put(key, newProperty);

        // Update edge payload in COW.
        if (!TrueGraphDBHelper.updateEdgePayload(this))
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
        if (!TrueGraphDBHelper.removeEdge(this))
        {
            throw new UnsupportedOperationException("COW edge deletion failed!");
        }

        final TrueGraphDBVertex inVertex = (TrueGraphDBVertex)this.inVertex;
        final TrueGraphDBVertex outVertex = (TrueGraphDBVertex)this.outVertex;

        if (inVertex != null && inVertex.inEdges != null)
        {
            final Set<Edge> edges = inVertex.inEdges.get(this.label);
            if (edges != null)
            {
                edges.remove(this);
            }
        }

        if (outVertex != null && outVertex.outEdges != null)
        {
            final Set<Edge> edges = outVertex.outEdges.get(this.label);
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
