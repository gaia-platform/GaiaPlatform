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

public final class TrueGraphDBVertexProperty<V> extends TrueGraphDBElement implements VertexProperty<V>
{
    private final TrueGraphDBVertex vertex;
    protected final String key;
    protected final V value;

    protected Map<String, Property> properties;

    protected TrueGraphDBVertexProperty(
        final TrueGraphDBVertex vertex,
        final String key, final V value,
        final Object... propertyKeyValues)
    {
        this(
            vertex.graph.vertexPropertyIdManager.getNextId(vertex.graph),
            vertex,
            key, value,
            propertyKeyValues);
    }

    public TrueGraphDBVertexProperty(
        final Object id,
        final TrueGraphDBVertex vertex,
        final String key, final V value,
        final Object... propertyKeyValues)
    {
        super(vertex.graph, id, key);

        this.vertex = vertex;
        this.key = key;
        this.value = value;

        ElementHelper.legalPropertyKeyValueArray(propertyKeyValues);
        ElementHelper.attachProperties(this, propertyKeyValues);
    }

    public Vertex element()
    {
        return this.vertex;
    }

    public String key()
    {
        return this.key;
    }

    public V value()
    {
        return this.value;
    }

    public boolean isPresent()
    {
        return true;
    }

    public Set<String> keys()
    {
        return (this.properties == null) ? Collections.emptySet() : this.properties.keySet();
    }

    public <U> Property<U> property(final String key, final U value)
    {
        if (this.removed)
        {
            throw elementAlreadyRemoved(VertexProperty.class, this.id);
        }

        ElementHelper.validateProperty(key, value);

        final Property<U> newProperty = new TrueGraphDBProperty<>(this, key, value);

        if (this.properties == null)
        {
            this.properties = new HashMap<>();
        }

        this.properties.put(key, newProperty);

        // TODO: Update node payload in COW.

        return newProperty;
    }

    public <U> Iterator<Property<U>> properties(final String... propertyKeys)
    {
        if (this.properties == null)
        {
            return Collections.emptyIterator();
        }

        if (propertyKeys.length == 1)
        {
            final Property<U> property = this.properties.get(propertyKeys[0]);
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
        if (this.vertex.properties != null && this.vertex.properties.containsKey(this.key))
        {
            this.vertex.properties.get(this.key).remove(this);
            if (this.vertex.properties.get(this.key).size() == 0)
            {
                this.vertex.properties.remove(this.key);
            }
        }        

        // TODO: Update node payload in COW.

        this.properties = null;
        this.removed = true;
    }

    public String toString()
    {
        return StringFactory.propertyString(this);
    }
}
