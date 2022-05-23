////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

package com.gaiaplatform.database.cachegraph.tinkerpop.gremlin.structure;

import org.apache.tinkerpop.gremlin.structure.Graph;
import org.apache.tinkerpop.gremlin.structure.Transaction;
import org.apache.tinkerpop.gremlin.structure.util.AbstractThreadedTransaction;

import com.gaiaplatform.database.GaiaDatabase;

// CacheTransaction only enables the management of transactions for operations with the underlying store.
// These transactions do not cover the operations with the in-memory CacheGraph data.
public final class CacheTransaction extends AbstractThreadedTransaction
{
    private CacheGraph graph;
    private boolean isOpen = false;

    public CacheTransaction(final Graph g)
    {
        super(g);

        if (!(g instanceof CacheGraph))
        {
            throw new UnsupportedOperationException("CacheTransaction can only be used with CacheGraph.");
        }

        this.graph = (CacheGraph)g;
    }

    public boolean isOpen()
    {
        return this.isOpen;
    }

    protected void doOpen()
    {
        CacheHelper.debugPrint(this.graph, "tx::open()");

        if (!graph.enableGaiaDbOperations)
        {
            return;
        }

        this.graph.gaiaDb.beginTransaction();
        this.isOpen = true;
    }

    protected void doCommit()
    {
        CacheHelper.debugPrint(this.graph, "tx::commit()");

        if (!graph.enableGaiaDbOperations)
        {
            return;
        }

        this.graph.gaiaDb.commitTransaction();
        this.isOpen = false;
    }

    protected void doRollback()
    {
        CacheHelper.debugPrint(this.graph, "tx::rollback()");

        if (!graph.enableGaiaDbOperations)
        {
            return;
        }

        this.graph.gaiaDb.rollbackTransaction();
        this.isOpen = false;
    }
}
