////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

package com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure;

import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;

public abstract class CacheElement implements Element
{
    protected final CacheGraph graph;
    protected final Object id;
    protected final String label;

    protected boolean supportsNullPropertyValues;

    protected boolean removed = false;

    protected CacheElement(final Graph graph, final Object id, final String label)
    {
        this.graph = (CacheGraph)graph;
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
