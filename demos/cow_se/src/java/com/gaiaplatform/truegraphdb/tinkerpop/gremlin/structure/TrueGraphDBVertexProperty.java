/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.stream.Collectors;

import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class TrueGraphDBVertexProperty<V> extends TrueGraphDBElement implements VertexProperty<V>
{
    private final TrueGraphDBVertex vertex;
    protected final String key;
    protected final V value;

    protected Map<String, Property> properties;

    protected TrueGraphDBVertexProperty(final TrueGraphDBVertex vertex,
        final String key, final V value, final Object... propertyKeyValues)
    {
        super(vertex.graph(), null, key);

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
        ElementHelper.validateProperty(key, value);

        final Property<U> newProperty = new TrueGraphDBProperty<>(this, key, value);

        if (this.properties == null)
        {
            this.properties = new HashMap<>();
        }

        this.properties.put(key, newProperty);

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
            return null == property ? Collections.emptyIterator() : IteratorUtils.of(property);
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

    public String toString()
    {
        return StringFactory.propertyString(this);
    }
}
