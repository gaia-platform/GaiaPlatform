/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/////////////////////////////////////////////
// Portions of this code are derived
// from TinkerGraph project.
// Used under Apache License 2.0
/////////////////////////////////////////////

package com.gaiaplatform.database.twingraph.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

public final class TwinVertexProperty<V> extends TwinElement implements VertexProperty<V>
{
    private final TwinVertex vertex;
    protected final String key;
    protected final V value;

    protected Map<String, Property> properties;

    protected TwinVertexProperty(
        final TwinVertex vertex,
        final String key, final V value,
        final Object... propertyKeyValues)
    {
        this(
            vertex.graph.vertexPropertyIdManager.getNextId(vertex.graph),
            vertex,
            key, value,
            propertyKeyValues);
    }

    public TwinVertexProperty(
        final Object id,
        final TwinVertex vertex,
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

        final Property<U> newProperty = new TwinProperty<>(this, key, value);

        if (this.properties == null)
        {
            this.properties = new ConcurrentHashMap<>();
        }

        this.properties.put(key, newProperty);

        // No plans to support vertex property properties in COW for now.
        // When we will, we will also need to update the node payload in COW here.

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

        // Update node payload in COW.
        if (!TwinHelper.updateNodePayload(this.vertex))
        {
            throw new UnsupportedOperationException("COW node update failed!");
        }

        this.properties = null;
        this.removed = true;
    }

    public String toString()
    {
        return StringFactory.propertyString(this);
    }
}
