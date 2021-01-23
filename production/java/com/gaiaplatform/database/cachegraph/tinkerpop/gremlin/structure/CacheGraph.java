/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/////////////////////////////////////////////
// Portions of this code are derived
// from TinkerGraph project.
// Used under Apache License 2.0
/////////////////////////////////////////////

package com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure;

import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
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
import org.apache.tinkerpop.gremlin.structure.Property;
import org.apache.tinkerpop.gremlin.structure.Transaction;
import org.apache.tinkerpop.gremlin.structure.Vertex;
import org.apache.tinkerpop.gremlin.structure.VertexProperty;
import org.apache.tinkerpop.gremlin.structure.util.ElementHelper;
import org.apache.tinkerpop.gremlin.structure.util.GraphVariableHelper;
import org.apache.tinkerpop.gremlin.structure.util.StringFactory;
import org.apache.tinkerpop.gremlin.tinkergraph.structure.TinkerGraphIterator;
import org.apache.tinkerpop.gremlin.tinkergraph.structure.TinkerGraphVariables;
import org.apache.tinkerpop.gremlin.util.iterator.IteratorUtils;

import com.gaiaplatform.database.GaiaDatabase;

@Graph.OptIn(Graph.OptIn.SUITE_STRUCTURE_STANDARD)
@Graph.OptIn(Graph.OptIn.SUITE_STRUCTURE_INTEGRATE)
@Graph.OptIn(Graph.OptIn.SUITE_PROCESS_STANDARD)
//@Graph.OptIn(Graph.OptIn.SUITE_PROCESS_COMPUTER)
public final class CacheGraph implements Graph
{
    private static final Configuration EMPTY_CONFIGURATION = new BaseConfiguration()
    {{
        this.setProperty(Graph.GRAPH, CacheGraph.class.getName());
    }};

    public static final String CACHEGRAPH_VERTEX_ID_MANAGER
        = "truegraphdb.vertexIdManager";
    public static final String CACHEGRAPH_EDGE_ID_MANAGER
        = "truegraphdb.edgeIdManager";
    public static final String CACHEGRAPH_VERTEX_PROPERTY_ID_MANAGER
        = "truegraphdb.vertexPropertyIdManager";
    public static final String CACHEGRAPH_DEFAULT_VERTEX_PROPERTY_CARDINALITY
        = "truegraphdb.defaultVertexPropertyCardinality";
    public static final String CACHEGRAPH_CREATE_ON_START
        = "truegraphdb.createOnStart";
    public static final String CACHEGRAPH_ENABLE_GAIADB_OPERATIONS
        = "truegraphdb.enableGaiaDbOperations";
    public static final String CACHEGRAPH_ENABLE_AIRPORT_CODE
        = "truegraphdb.enableAirportCode";
    public static final String CACHEGRAPH_ENABLE_DEBUG_MESSAGES
        = "truegraphdb.enableDebugMessages";

    private final CacheFeatures features = new CacheFeatures();

    private CacheTransaction transaction = new CacheTransaction(this);

    // Reuse TinkerGraph's Graph.Variables implementation.
    protected TinkerGraphVariables variables = null;

    protected AtomicLong lastId = new AtomicLong();
    protected final IdManager<?> vertexIdManager;
    protected final IdManager<?> edgeIdManager;
    protected final IdManager<?> vertexPropertyIdManager;

    protected final VertexProperty.Cardinality defaultVertexPropertyCardinality;
    protected final boolean supportsNullPropertyValues;

    private final Configuration configuration;

    protected Map<Object, Vertex> vertices = new ConcurrentHashMap<>();
    protected Map<Object, Edge> edges = new ConcurrentHashMap<>();

    protected GaiaDatabase gaiaDb = new GaiaDatabase();

    protected boolean enableGaiaDbOperations;
    protected boolean enableAirportCode;
    protected boolean enableDebugMessages;

    private CacheGraph(final Configuration configuration)
    {
        this.configuration = configuration;

        this.vertexIdManager = selectIdManager(
            configuration, CACHEGRAPH_VERTEX_ID_MANAGER, Vertex.class);
        this.edgeIdManager = selectIdManager(
            configuration, CACHEGRAPH_EDGE_ID_MANAGER, Edge.class);
        this.vertexPropertyIdManager = selectIdManager(
            configuration, CACHEGRAPH_VERTEX_PROPERTY_ID_MANAGER, VertexProperty.class);

        String cardinalitySetting = configuration.getString(
            CACHEGRAPH_DEFAULT_VERTEX_PROPERTY_CARDINALITY,
            VertexProperty.Cardinality.single.name());
        this.defaultVertexPropertyCardinality = VertexProperty.Cardinality.valueOf(
            cardinalitySetting);
        // Current implementation does not support null values.
        this.supportsNullPropertyValues = false;

        boolean createOnStart = configuration.getBoolean(
            CACHEGRAPH_CREATE_ON_START, true);

        this.enableGaiaDbOperations = configuration.getBoolean(
            CACHEGRAPH_ENABLE_GAIADB_OPERATIONS, true);
        this.enableAirportCode = configuration.getBoolean(
            CACHEGRAPH_ENABLE_AIRPORT_CODE, false);
        this.enableDebugMessages = configuration.getBoolean(
            CACHEGRAPH_ENABLE_DEBUG_MESSAGES, false);

        if (createOnStart)
        {
            CacheHelper.reset();
        }

        if (!this.enableAirportCode)
        {
            throw new UnsupportedOperationException("Opening of Gaia database is only supported for airport data!");
        }

        this.gaiaDb.beginSession();

        CacheHelper.loadAirportGraphFromGaiaDb(this);
    }

    public static CacheGraph open()
    {
        return open(EMPTY_CONFIGURATION);
    }

    public static CacheGraph open(final Configuration configuration)
    {
        return new CacheGraph(configuration);
    }

    public Vertex addVertex(final Object... keyValues)
    {
        CacheHelper.debugPrint(this, "graph::addVertex()");

        ElementHelper.legalPropertyKeyValueArray(keyValues);

        Object idValue = this.vertexIdManager.convert(
            ElementHelper.getIdValue(keyValues).orElse(null));
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

        final Vertex vertex = new CacheVertex(this, idValue, label);
        ElementHelper.attachProperties(vertex, VertexProperty.Cardinality.list, keyValues);

        // Create node in Gaia database.
        if (!CacheHelper.createNode((CacheVertex)vertex))
        {
            throw new UnsupportedOperationException("Gaia database node creation failed!");
        }

        this.vertices.put(vertex.id(), vertex);

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
        throw new UnsupportedOperationException("CacheGraph does not support Tinkerpop OLAP interfaces.");
    }

    public Graph.Variables variables()
    {
        if (this.variables == null)
        {
            this.variables = new TinkerGraphVariables();
        }

        return this.variables;
    }

    public Iterator<Vertex> vertices(final Object... vertexIds)
    {
        return createElementIterator(Vertex.class, vertices, vertexIdManager, vertexIds);
    }

    public Iterator<Edge> edges(final Object... edgeIds)
    {
        return createElementIterator(Edge.class, edges, edgeIdManager, edgeIds);
    }

    public Transaction tx()
    {
        return this.transaction;
    }

    public void close()
    {
        // Nothing to do here yet.
    }

    public Configuration configuration()
    {
        return this.configuration;
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

    private <T extends Element> Iterator<T> createElementIterator(
        final Class<T> clazz,
        final Map<Object, T> elements,
        final IdManager idManager,
        final Object... ids)
    {
        if (ids.length == 0)
        {
            return new TinkerGraphIterator<T>(elements.values().iterator());
        }
        else
        {
            final List<Object> idList = Arrays.asList(ids);
            validateHomogenousIds(idList);

            return clazz.isAssignableFrom(ids[0].getClass())
                ? new TinkerGraphIterator<T>(IteratorUtils.filter(
                    IteratorUtils.map(idList, id -> elements.get(clazz.cast(id).id())).iterator(),
                    Objects::nonNull))
                : new TinkerGraphIterator<T>(IteratorUtils.filter(
                    IteratorUtils.map(idList, id -> elements.get(idManager.convert(id))).iterator(),
                    Objects::nonNull));
        }
    }

    private void validateHomogenousIds(final List<Object> ids)
    {
        final Iterator<Object> iterator = ids.iterator();

        Object id = iterator.next();
        if (id == null)
        {
            throw Graph.Exceptions.idArgsMustBeEitherIdOrElement();
        }
        final Class firstIdClass = id.getClass();

        while (iterator.hasNext())
        {
            id = iterator.next();
            if (id == null || !id.getClass().equals(firstIdClass))
            {
                throw Graph.Exceptions.idArgsMustBeEitherIdOrElement();
            }
        }
    }

    public class CacheFeatures implements Features
    {
        private final CacheGraphFeatures graphFeatures = new CacheGraphFeatures();
        private final CacheEdgeFeatures edgeFeatures = new CacheEdgeFeatures();
        private final CacheVertexFeatures vertexFeatures = new CacheVertexFeatures();

        private CacheFeatures()
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

    public class CacheVertexFeatures implements Features.VertexFeatures
    {
        private final CacheVertexPropertyFeatures vertexPropertyFeatures
            = new CacheVertexPropertyFeatures();

        private CacheVertexFeatures()
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
            return defaultVertexPropertyCardinality;
        }
    }

    public class CacheEdgeFeatures implements Features.EdgeFeatures
    {
        private CacheEdgeFeatures()
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

    public class CacheGraphFeatures implements Features.GraphFeatures
    {
        private CacheGraphFeatures()
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
            // CacheTransaction only handles Gaia database transactions,
            // so we can't declare support yet.
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

    public class CacheVertexPropertyFeatures implements Features.VertexPropertyFeatures
    {
        private CacheVertexPropertyFeatures()
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
                    "Could not configure CacheGraph %s id manager with %s",
                    clazz.getSimpleName(),
                    idManagerConfigValue));
            }
        }
    }

    public interface IdManager<T>
    {
        T getNextId(final CacheGraph graph);

        T convert(final Object id);

        boolean allow(final Object id);
    }

    public enum DefaultIdManager implements IdManager
    {
        LONG
        {
            public Long getNextId(final CacheGraph graph)
            {
                return Stream.generate(() -> (graph.lastId.incrementAndGet()))
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
            public Integer getNextId(final CacheGraph graph)
            {
                return Stream.generate(() -> (graph.lastId.incrementAndGet()))
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
            public UUID getNextId(final CacheGraph graph)
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
            public Long getNextId(final CacheGraph graph)
            {
                return Stream.generate(() -> (graph.lastId.incrementAndGet()))
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
