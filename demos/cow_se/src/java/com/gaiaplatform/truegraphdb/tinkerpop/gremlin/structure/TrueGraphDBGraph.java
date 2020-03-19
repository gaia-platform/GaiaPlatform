/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/////////////////////////////////////////////
// Portions of this code are copied
// from TrueGraphDBGraph project.
// Used under Apache License 2.0
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb.tinkerpop.gremlin.structure;

import java.util.Collections;
import java.util.Iterator;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Stream;

import org.apache.commons.configuration.BaseConfiguration;
import org.apache.commons.configuration.Configuration;

import org.apache.tinkerpop.gremlin.process.computer.GraphComputer;
import org.apache.tinkerpop.gremlin.structure.Edge;
import org.apache.tinkerpop.gremlin.structure.Element;
import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Transaction;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.GraphVariableHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;

//@Graph.OptIn(Graph.OptIn.SUITE_STRUCTURE_STANDARD)
//@Graph.OptIn(Graph.OptIn.SUITE_STRUCTURE_INTEGRATE)
//@Graph.OptIn(Graph.OptIn.SUITE_PROCESS_STANDARD)
//@Graph.OptIn(Graph.OptIn.SUITE_PROCESS_COMPUTER)
public final class TrueGraphDBGraph implements Graph
{
    private static final Configuration EMPTY_CONFIGURATION = new BaseConfiguration()
    {{
        this.setProperty(Graph.GRAPH, TrueGraphDBGraph.class.getName());
    }};

    public static final String TRUEGRAPHDB_VERTEX_ID_MANAGER = "truegraphdb.vertexIdManager";
    public static final String TRUEGRAPHDB_EDGE_ID_MANAGER = "truegraphdb.edgeIdManager";
    public static final String TRUEGRAPHDB_VERTEX_PROPERTY_ID_MANAGER = "truegraphdb.vertexPropertyIdManager";

    private final TrueGraphDBFeatures features = new TrueGraphDBFeatures();

    protected TrueGraphDBVariables variables = null;

    protected AtomicLong currentId = new AtomicLong(-1L);
    protected final IdManager<?> vertexIdManager;
    protected final IdManager<?> edgeIdManager;
    protected final IdManager<?> vertexPropertyIdManager;

    private final Configuration configuration;

    protected Map<Object, Vertex> vertices = new ConcurrentHashMap<>();
    protected Map<Object, Edge> edges = new ConcurrentHashMap<>();

    private TrueGraphDBGraph(final Configuration configuration)
    {
        this.configuration = configuration;

        this.vertexIdManager = selectIdManager(
            configuration, TRUEGRAPHDB_VERTEX_ID_MANAGER, Vertex.class);
        this.edgeIdManager = selectIdManager(
            configuration, TRUEGRAPHDB_EDGE_ID_MANAGER, Edge.class);
        this.vertexPropertyIdManager = selectIdManager(
            configuration, TRUEGRAPHDB_VERTEX_PROPERTY_ID_MANAGER, VertexProperty.class);
    }

    public static TrueGraphDBGraph open()
    {
        return open(EMPTY_CONFIGURATION);
    }

    public static TrueGraphDBGraph open(final Configuration configuration)
    {
        return new TrueGraphDBGraph(configuration);
    }

    public Vertex addVertex(final Object... keyValues)
    {
        ElementHelper.legalPropertyKeyValueArray(keyValues);

        Object idValue = ElementHelper.getIdValue(keyValues).orElse(null);
        final String label = ElementHelper.getLabelValue(keyValues).orElse(Vertex.DEFAULT_LABEL);

        if (idValue != null)
        {
            if (this.vertices.containsKey(idValue))
            {
                throw Exceptions.vertexWithIdAlreadyExists(idValue);
            }
        }
        else
        {
            idValue = this.vertexIdManager.getNextId(this);
        }

        // TODO: Create database vertex in COW.

        final Vertex vertex = new TrueGraphDBVertex(this, idValue, label);
        this.vertices.put(vertex.id(), vertex);

        ElementHelper.attachProperties(vertex, VertexProperty.Cardinality.list, keyValues);

        return vertex;
    }

    public <C extends GraphComputer> C compute(final Class<C> graphComputerClass)
    throws IllegalArgumentException
    {
        throw Graph.Exceptions.graphDoesNotSupportProvidedGraphComputer(graphComputerClass);
    }

    public GraphComputer compute()
    throws IllegalArgumentException
    {
        throw new UnsupportedOperationException();
    }

    public Iterator<Vertex> vertices(final Object... vertexIds)
    {
        // TODO: Add implementation.

        return Collections.emptyIterator();
    }

    public Iterator<Edge> edges(final Object... edgeIds)
    {
        // TODO: Add implementation.

        return Collections.emptyIterator();
    }

    public Transaction tx()
    {
        throw Exceptions.transactionsNotSupported();
    }

    public void close()
    {
        // Nothing to do here yet.
    }

    public Graph.Variables variables()
    {
        if (this.variables == null)
        {
            this.variables = new TrueGraphDBVariables();
        }

        return this.variables;
    }

    public Configuration configuration()
    {
        return configuration;
    }

    public Features features()
    {
        return this.features;
    }

    public String toString()
    {
        return StringFactory.graphString(
            this, "vertices:" + this.vertices.size() + " edges:" + this.edges.size());
    }

    public final class TrueGraphDBVariables implements Graph.Variables
    {
        private final Map<String, Object> variables = new ConcurrentHashMap<>();
    
        public TrueGraphDBVariables()
        {
        }
    
        public Set<String> keys()
        {
            return this.variables.keySet();
        }
    
        public <R> Optional<R> get(final String key)
        {
            return Optional.ofNullable((R)this.variables.get(key));
        }
    
        public void remove(final String key)
        {
            this.variables.remove(key);
        }
    
        public void set(final String key, final Object value)
        {
            GraphVariableHelper.validateVariable(key, value);

            this.variables.put(key, value);
        }
    
        public String toString()
        {
            return StringFactory.graphVariablesString(this);
        }
    }

    public class TrueGraphDBFeatures implements Features
    {
        private final TrueGraphDBGraphFeatures graphFeatures = new TrueGraphDBGraphFeatures();
        private final TrueGraphDBEdgeFeatures edgeFeatures = new TrueGraphDBEdgeFeatures();
        private final TrueGraphDBVertexFeatures vertexFeatures = new TrueGraphDBVertexFeatures();

        private TrueGraphDBFeatures()
        {
        }

        public GraphFeatures graph()
        {
            return graphFeatures;
        }

        public EdgeFeatures edge()
        {
            return edgeFeatures;
        }

        public VertexFeatures vertex()
        {
            return vertexFeatures;
        }
    }

    public class TrueGraphDBVertexFeatures implements Features.VertexFeatures
    {
        private final TrueGraphDBVertexPropertyFeatures vertexPropertyFeatures
            = new TrueGraphDBVertexPropertyFeatures();

        private TrueGraphDBVertexFeatures()
        {
        }

        public Features.VertexPropertyFeatures properties()
        {
            return vertexPropertyFeatures;
        }

        public boolean supportsMultiProperties()
        {
            return false;
        }

        public boolean supportsMetaProperties()
        {
            return false;
        }

        public boolean supportsStringIds()
        {
            return false;
        }

        public boolean supportsUuidIds()
        {
            return false;
        }

        public boolean supportsCustomIds()
        {
            return false;
        }

        public boolean willAllowId(final Object id)
        {
            return vertexIdManager.allow(id);
        }

        public VertexProperty.Cardinality getCardinality(final String key)
        {
            return VertexProperty.Cardinality.list;
        }
    }

    public class TrueGraphDBEdgeFeatures implements Features.EdgeFeatures
    {
        private TrueGraphDBEdgeFeatures()
        {
        }

        public boolean supportsStringIds()
        {
            return false;
        }

        public boolean supportsUuidIds()
        {
            return false;
        }

        public boolean supportsCustomIds()
        {
            return false;
        }

        public boolean willAllowId(final Object id)
        {
            return edgeIdManager.allow(id);
        }
    }

    public class TrueGraphDBGraphFeatures implements Features.GraphFeatures
    {
        private TrueGraphDBGraphFeatures()
        {
        }

        public boolean supportsComputer()
        {
            return false;
        }

        public boolean supportsPersistence()
        {
            return false;
        }

        public boolean supportsConcurrentAccess()
        {
            return false;
        }

        public boolean supportsTransactions()
        {
            return false;
        }

        public boolean supportsThreadedTransactions()
        {
            return false;
        }

        public boolean supportsIoRead()
        {
            return false;
        }

        public boolean supportsIoWrite()
        {
            return false;
        }
}

    public class TrueGraphDBVertexPropertyFeatures implements Features.VertexPropertyFeatures
    {
        private TrueGraphDBVertexPropertyFeatures()
        {
        }

        public boolean supportsUserSuppliedIds()
        {
            return false;
        }

        public boolean supportsCustomIds()
        {
            return false;
        }

        public boolean willAllowId(final Object id)
        {
            return true;
        }
    }    

    private static IdManager<?> selectIdManager(
        final Configuration configuration,
        final String configurationKey,
        final Class<? extends Element> clazz)
    {
        final String idManagerConfigValue = configuration.getString(
            configurationKey, DefaultIdManager.ANY.name());
        
        try
        {
            return DefaultIdManager.valueOf(idManagerConfigValue);
        }
        catch (IllegalArgumentException iae)
        {
            try
            {
                return (IdManager)Class.forName(idManagerConfigValue).newInstance();
            }
            catch (Exception e)
            {
                throw new IllegalStateException(String.format(
                    "Could not configure TrueGraphDBGraph %s id manager with %s",
                    clazz.getSimpleName(),
                    idManagerConfigValue));
            }
        }
    }

    public interface IdManager<T>
    {
        T getNextId(final TrueGraphDBGraph graph);

        T convert(final Object id);

        boolean allow(final Object id);
    }

    public enum DefaultIdManager implements IdManager
    {
        LONG
        {
            public Long getNextId(final TrueGraphDBGraph graph)
            {
                return Stream.generate(() -> (graph.currentId.incrementAndGet()))
                    .filter(id -> !graph.vertices.containsKey(id) && !graph.edges.containsKey(id))
                    .findAny().get();
            }

            public Object convert(final Object id)
            {
                if (id == null)
                {
                    return null;
                }
                else if (id instanceof Long)
                {
                    return id;
                }
                else if (id instanceof Number)
                {
                    return ((Number)id).longValue();
                }
                else if (id instanceof String)
                {
                    try
                    {
                        return Long.parseLong((String) id);
                    }
                    catch (NumberFormatException nfe)
                    {
                        throw new IllegalArgumentException(createErrorMessage(Long.class, id));
                    }
                }
                else
                {
                    throw new IllegalArgumentException(createErrorMessage(Long.class, id));
                }
            }

            public boolean allow(final Object id)
            {
                return id instanceof Number || id instanceof String;
            }
        },

        INTEGER
        {
            public Integer getNextId(final TrueGraphDBGraph graph)
            {
                return Stream.generate(() -> (graph.currentId.incrementAndGet()))
                    .map(Long::intValue)
                    .filter(id -> !graph.vertices.containsKey(id) && !graph.edges.containsKey(id))
                    .findAny().get();
            }

            public Object convert(final Object id)
            {
                if (id == null)
                {
                    return null;
                }
                else if (id instanceof Integer)
                {
                    return id;
                }
                else if (id instanceof Number)
                {
                    return ((Number)id).intValue();
                }
                else if (id instanceof String)
                {
                    try
                    {
                        return Integer.parseInt((String) id);
                    }
                    catch (NumberFormatException nfe)
                    {
                        throw new IllegalArgumentException(createErrorMessage(Integer.class, id));
                    }
                }
                else
                {
                    throw new IllegalArgumentException(createErrorMessage(Integer.class, id));
                }
            }

            public boolean allow(final Object id)
            {
                return id instanceof Number || id instanceof String;
            }
        },

        UUID
        {
            public UUID getNextId(final TrueGraphDBGraph graph)
            {
                return java.util.UUID.randomUUID();
            }

            public Object convert(final Object id)
            {
                if (id == null)
                {
                    return null;
                }
                else if (id instanceof java.util.UUID)
                {
                    return id;
                }
                else  if (id instanceof String)
                {
                    try
                    {
                        return java.util.UUID.fromString((String) id);
                    }
                    catch (IllegalArgumentException iae)
                    {
                        throw new IllegalArgumentException(createErrorMessage(java.util.UUID.class, id));
                    }
                }
                else
                {
                    throw new IllegalArgumentException(createErrorMessage(java.util.UUID.class, id));
                }
            }

            public boolean allow(final Object id)
            {
                return id instanceof UUID || id instanceof String;
            }
        },

        ANY
        {
            public Long getNextId(final TrueGraphDBGraph graph)
            {
                return Stream.generate(() -> (graph.currentId.incrementAndGet()))
                    .filter(id -> !graph.vertices.containsKey(id) && !graph.edges.containsKey(id))
                    .findAny().get();
            }

            public Object convert(final Object id)
            {
                return id;
            }

            public boolean allow(final Object id)
            {
                return true;
            }
        };

        private static String createErrorMessage(final Class<?> expectedType, final Object id)
        {
            return String.format(
                "Expected an id that is convertible to %s but received %s - [%s]",
                expectedType,
                id.getClass(),
                id);
        }
    }    
}
