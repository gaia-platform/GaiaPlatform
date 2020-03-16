/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;

import org.apache.commons.configuration.BaseConfiguration;
import org.apache.commons.configuration.Configuration;

import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.Transaction;

import org.apache.tinkerpop.gremlin.process.computer.GraphComputer;

public final class TrueGraphDBGraph implements Graph
{
    public Vertex addVertex(final Object... keyValues)
    {
        return null;
    }

    public <C extends GraphComputer> C compute(final Class<C> graphComputerClass)
    throws IllegalArgumentException
    {
        throw Graph.Exceptions.graphDoesNotSupportProvidedGraphComputer(graphComputerClass);
    }

    public GraphComputer compute()
    throws IllegalArgumentException
    {
        return null;
    }

    public Iterator<Vertex> vertices(final Object... vertexIds)
    {
        return Collections.emptyIterator();
    }

    public Iterator<Edge> edges(final Object... edgeIds)
    {
        return Collections.emptyIterator();
    }

    public Transaction tx()
    {
        throw Exceptions.transactionsNotSupported();
    }

    public void close()
    throws Exception
    {
    }

    public Graph.Variables variables()
    {
        return null;
    }

    public Configuration configuration()
    {
        return null;
    }
}
