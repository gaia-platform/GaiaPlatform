/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.database.twingraph.tinkerpop.gremlin.structure;

import java.util.NoSuchElementException;

import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;

public final class TwinProperty<V> implements Property<V>
{
    protected final Element element;
    protected final String key;
    protected final V value;

    public TwinProperty(Element element, String key, V value)
    {
        this.element = element;
        this.key = key;
        this.value = value;
    }

    public Element element()
    {
        return this.element;
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

    public void remove()
    {
        if (this.element instanceof Edge)
        {
            ((TwinEdge)this.element).properties.remove(this.key);

            // Update edge payload in COW.
            if (!TwinHelper.updateEdgePayload((TwinEdge)this.element))
            {
                throw new UnsupportedOperationException("COW edge update failed!");
            }
        }
        else
        {
            ((TwinVertexProperty)this.element).properties.remove(this.key);

            // No plans to support vertex property properties in COW for now.
        }
    }

    public String toString()
    {
        return StringFactory.propertyString(this);
    }

    public int hashCode()
    {
        return ElementHelper.hashCode(this);
    }

    public boolean equals(final Object object)
    {
        return ElementHelper.areEqual(this, object);
    }
}
