/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.stream.Collectors;

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;

import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class TrueGraphDBEdge extends TrueGraphDBElement implements Edge
{
    protected final Vertex inVertex;
    protected final Vertex outVertex;
    protected Map<String, Property> properties;

    protected TrueGraphDBEdge(final Object id, final String label,
        final Vertex inVertex, final Vertex outVertex)
    {
        super(inVertex.graph(), id, label);

        this.inVertex = inVertex;
        this.outVertex = outVertex;
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

    public <V> Property<V> property(final String key, final V value)
    {
        ElementHelper.validateProperty(key, value);

        final Property<V> newProperty = new TrueGraphDBProperty<>(this, key, value);

        if (this.properties == null)
        {
            this.properties = new HashMap<>();
        }

        this.properties.put(key, newProperty);

        return newProperty;
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
        this.removed = true;
    }
}
