/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.NoSuchElementException;

import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Property;

public final class TrueGraphDBProperty<V> implements Property<V>
{
    protected final Element element;
    protected final String key;
    protected final V value;

    public TrueGraphDBProperty(Element element, String key, V value)
    {
        this.element = element;
        this.key = key;
        this.value = value;
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

    public Element element()
    {
        return this.element;
    }

    public void remove()
    {
    }
}
