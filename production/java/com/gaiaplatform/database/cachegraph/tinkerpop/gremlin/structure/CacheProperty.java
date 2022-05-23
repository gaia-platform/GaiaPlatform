////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

package com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure;

import java.util.NoSuchElementException;

import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;

public final class CacheProperty<V> implements Property<V>
{
    protected final Element element;
    protected final String key;
    protected final V value;

    public CacheProperty(Element element, String key, V value)
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
            ((CacheEdge)this.element).properties.remove(this.key);

            // Update edge payload in Gaia database.
            if (!CacheHelper.updateEdgePayload((CacheEdge)this.element))
            {
                throw new UnsupportedOperationException("Gaia database edge update failed!");
            }
        }
        else
        {
            ((CacheVertexProperty)this.element).properties.remove(this.key);

            // No plans to support vertex property properties in Gaia database for now.
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
