/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;

import org.apache.tinkerpop.gremlin.structure.Direction;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Vertex;

public final class TrueGraphDBEdge extends TrueGraphDBElement implements Edge
{
    public Iterator<Vertex> vertices(final Direction direction)
    {
        return Collections.emptyIterator();
    }

    public <V> Property<V> property(final String key, final V value)
    {
        return null;
    }

    public <V> Iterator<Property<V>> properties(final String... propertyKeys)
    {
        return Collections.emptyIterator();
    }
}
