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
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;

public final class TrueGraphDBVertex extends TrueGraphDBElement implements Vertex
{
    public Edge addEdge(final String label, final Vertex inVertex, final Object... keyValues)
    {
        if (inVertex == null)
        {
            throw Graph.Exceptions.argumentCanNotBeNull("inVertex");
        }

        return null;
    }

    public <V> VertexProperty<V> property(
        final VertexProperty.Cardinality cardinality,
        final String key,
        final V value,
        final Object... keyValues)
    {
        return null;
    }

    public Iterator<Edge> edges(final Direction direction, final String... edgeLabels)
    {
        return Collections.emptyIterator();
    }

    public Iterator<Vertex> vertices(final Direction direction, final String... edgeLabels)
    {
        return Collections.emptyIterator();
    }

    public <V> Iterator<VertexProperty<V>> properties(final String... propertyKeys)
    {
        if (removed)
        {
            return Collections.emptyIterator();
        }

        return Collections.emptyIterator();
    }
}
