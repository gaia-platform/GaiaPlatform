/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;
import java.util.Set;
import java.util.StringTokenizer;

import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Property;

public abstract class TrueGraphDBElement implements Element
{
    protected final Graph graph;
    protected final Object id;
    protected final String label;
    protected String payload;
    protected boolean removed = false;

    protected TrueGraphDBElement(final Graph graph, final Object id, final String label)
    {
        this.graph = graph;
        this.id = id;
        this.label = label;
    }

    public Graph graph()
    {
        return null;
    }

    public Object id()
    {
        return this.id;
    }

    public String label()
    {
        return this.label;
    }
}
