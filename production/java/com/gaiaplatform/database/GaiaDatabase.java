/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.database;

public class GaiaDatabase
{
    static
    {
        System.loadLibrary("gaia_db_jni");
    }

    public static void main(String[] args)
    {
        if (args.length == 0)
        {
            runDatabaseTest();
        }
        else if (args[0] != null && args[0].toLowerCase().equals("dump"))
        {
            dumpDatabase();
        }
        else
        {
            System.out.println("The only valid argument is 'dump'!");
        }
    }

    public static void dumpDatabase()
    {
        GaiaDatabase gaiaDb = new GaiaDatabase();

        gaiaDb.beginSession();

        gaiaDb.beginTransaction();

        long currentType = 1;
        while (true)
        {
            System.out.println("\n>>> Looking for entities of type " + currentType + "...");

            boolean hasFoundNode = false;

            long currentNodeId = gaiaDb.findFirstNode(currentType);
            while (currentNodeId != 0)
            {
                if (!hasFoundNode)
                {
                    hasFoundNode = true;
                    System.out.println(">>> Found nodes:");
                }

                gaiaDb.printNode(currentNodeId);
                currentNodeId = gaiaDb.findNextNode(currentNodeId);
            }

            if (!hasFoundNode)
            {
                System.out.println(
                    ">>> Could not find a node of type " + currentType
                    + "! Will conclude storage scan.");
                break;
            }

            ++currentType;
        }

        // We're not making any changes to gaiaDb, so we can just rollback.
        gaiaDb.rollbackTransaction();

        gaiaDb.endSession();
    }

    private static void runDatabaseTest()
    {
        GaiaDatabase gaiaDb = new GaiaDatabase();

        gaiaDb.beginSession();

        // First transaction - dump first 4 nodes (they must exist for catalog objects).
        gaiaDb.beginTransaction();

        long nodeId1 = 1;
        long nodeId2 = 2;
        long nodeId3 = 3;
        long nodeId4 = 4;

        gaiaDb.printNode(nodeId1);
        gaiaDb.printNode(nodeId2);
        gaiaDb.printNode(nodeId3);
        gaiaDb.printNode(nodeId4);
        printEmptyLine();

        gaiaDb.commitTransaction();

        // Second transaction - iterate through nodes of type 'database'.
        gaiaDb.beginTransaction();

        System.out.println("Iterating through nodes of type 'database' (-5 = 4294967291): ");
        long currentNodeId = gaiaDb.findFirstNode(-5);
        while (currentNodeId != 0)
        {
            gaiaDb.printNode(currentNodeId);
            currentNodeId = gaiaDb.findNextNode(currentNodeId);
        }
        printEmptyLine();

        gaiaDb.commitTransaction();

        gaiaDb.endSession();

        System.out.println("All tests passed!");
    }

    private static void printEmptyLine()
    {
        System.out.println("");
    }

    private static void printPayload(byte[] payload)
    {
        if (payload.length == 0)
        {
            return;
        }

        final char LOWEST_PRINTABLE_CHARACTER = 32;
        final char HIGHEST_PRINTABLE_CHARACTER = 126;

        System.out.print(" Payload: ");
        for (int i = 0; i < payload.length; i++)
        {
            char character = (char)payload[i];
            if (character >= LOWEST_PRINTABLE_CHARACTER && character <= HIGHEST_PRINTABLE_CHARACTER)
            {
                System.out.print(character);
            }
            else
            {
                System.out.print(".");
            }
        }
    }

    public void printEdge(long edgeId)
    {
        printEdge(edgeId, false);
    }

    public void printEdge(long edgeId, boolean indent)
    {
        printEmptyLine();

        if (indent)
        {
            System.out.print("  ");
        }

        if (edgeId == 0)
        {
            System.out.print("<null_edge>");
            return;
        }

        System.out.print("Edge id:" + edgeId + ", type:" + getEdgeType(edgeId));

        printPayload(getEdgePayload(edgeId));

        if (!indent)
        {
            printNode(getEdgeFirstNode(edgeId), true);
            printNode(getEdgeSecondNode(edgeId), true);
            printEmptyLine();
        }
        else
        {
            System.out.print(" first: " + getEdgeFirstNode(edgeId) + " second: " + getEdgeSecondNode(edgeId));
        }
    }

    public void printNode(long nodeId)
    {
        printNode(nodeId, false);
    }

    public void printNode(long nodeId, boolean indent)
    {
        printEmptyLine();

        if (indent)
        {
            System.out.print("  ");
        }

        if (nodeId == 0)
        {
            System.out.print("<null_node>");
            return;
        }

        System.out.print("Node id:" + nodeId + ", type:" + getNodeType(nodeId));

        printPayload(getNodePayload(nodeId));

        if (indent)
        {
            return;
        }

        printEmptyLine();
    }

    public long createNode(long id, long type, String payload)
    {
        return createNode(id, type, payload.getBytes());
    }

    public boolean updateNodePayload(long id, String payload)
    {
        return updateNodePayload(id, payload.getBytes());
    }

    public long createEdge(long id, long type, long idFirstNode, long idSecondNode, String payload)
    {
        return createEdge(id, type, idFirstNode, idSecondNode, payload.getBytes());
    }

    public boolean updateEdgePayload(long id, String payload)
    {
        return updateEdgePayload(id, payload.getBytes());
    }

    // Native interface.
    public native void beginSession();
    public native void endSession();

    public native void beginTransaction();
    public native void commitTransaction();
    public native void rollbackTransaction();

    public native long createNode(long id, long type, byte[] payload);
    public native boolean updateNodePayload(long id, byte[] payload);
    public native boolean removeNode(long id);

    public native long findFirstNode(long type);
    public native long findNextNode(long id);

    public native long getNodeType(long id);
    public native byte[] getNodePayload(long id);

    // Dummy implementations for outdated interfaces.
    public long getFirstEdgeWithNodeAsFirst(long id) { return 0; }
    public long getFirstEdgeWithNodeAsSecond(long id) { return 0; }

    public long createEdge(long id, long type, long idFirstNode, long idSecondNode, byte[] payload) { return 0; }
    public boolean updateEdgePayload(long id, byte[] payload) { return false; }
    public boolean removeEdge(long id) { return false; }

    public long findFirstEdge(long type) { return 0; }
    public long findNextEdge(long id) { return 0; }

    public long getEdgeType(long id) { return 0; }
    public long getEdgeFirstNode(long id) { return 0; }
    public long getEdgeSecondNode(long id) { return 0; }
    public byte[] getEdgePayload(long id) { return new byte[1]; }

    public long getNextEdgeWithSameFirstNode(long id) { return 0; }
    public long getNextEdgeWithSameSecondNode(long id) { return 0; }
}
