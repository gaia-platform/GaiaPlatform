/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;
import java.util.Set;

import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;

public abstract class TrueGraphDBElement implements Element
{
    protected boolean removed = false;

    public Graph graph()
    {
        return null;
    }

    public Object id()
    {
        return null;
    }

    public String label()
    {
        return null;
    }

    public void remove()
    {
        removed = true;
    }
}
