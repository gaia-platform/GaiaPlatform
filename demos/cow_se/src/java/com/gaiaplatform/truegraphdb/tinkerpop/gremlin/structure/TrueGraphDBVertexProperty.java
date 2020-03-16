/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;
import java.util.NoSuchElementException;

import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;

public final class TrueGraphDBVertexProperty<V> extends TrueGraphDBElement implements VertexProperty<V>
{
    public String key()
    {
        return null;
    }

    public V value()
    throws NoSuchElementException
    {
        return null;
    }

    public boolean isPresent()
    {
        return false;
    }

    public Vertex element()
    {
        return null;
    }

    public void remove()
    {
    }

    public <U> Property<U> property(final String key, final U value)
    {
        return null;
    }

    public <U> Iterator<Property<U>> properties(final String... propertyKeys)
    {
        return Collections.emptyIterator();
    }
}
