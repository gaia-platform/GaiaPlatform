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

    public Element element()
    {
        return null;
    }

    public void remove()
    {
    }
}
