/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.database.twingraph.tinkerpop.gremlin.structure;

import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;

public abstract class TwinElement implements Element
{
    protected final TwinGraph graph;
    protected final Object id;
    protected final String label;
    protected boolean removed = false;

    protected TwinElement(final Graph graph, final Object id, final String label)
    {
        this.graph = (TwinGraph)graph;
        this.id = id;
        this.label = label;
    }

    public Graph graph()
    {
        return this.graph;
    }

    public Object id()
    {
        return this.id;
    }

    public String label()
    {
        return this.label;
    }

    public int hashCode()
    {
        return ElementHelper.hashCode(this);
    }

    public boolean equals(final Object object)
    {
        return ElementHelper.areEqual(this, object);
    }

    protected static IllegalStateException elementAlreadyRemoved(
        final Class<? extends Element> clazz, final Object id)
    {
        return new IllegalStateException(
            String.format("%s with id %s was already removed.", clazz.getSimpleName(), id));
    }
}
