/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.database;

public class CowStorageEngine
{
    static
    {
        System.loadLibrary("jni_db_client");
    }

    public static void main(String[] args)
    {
        if (args.length == 0)
        {
            runCowTest();
        }
        else if (args[0] != null && args[0].toLowerCase().equals("dump"))
        {
            dumpCow();
        }
        else
        {
            System.out.println("The only valid argument is 'dump'!");
        }
    }

    public static void dumpCow()
    {
        CowStorageEngine cow = new CowStorageEngine();

        cow.beginSession();

        cow.beginTransaction();

        long currentType = 1;
        while (true)
        {
            System.out.println("\n>>> Looking for entities of type " + currentType + "...");

            boolean hasFoundNode = false;
            boolean hasFoundEdge = false;

            long currentNodeId = cow.findFirstNode(currentType);
            while (currentNodeId != 0)
            {
                if (!hasFoundNode)
                {
                    hasFoundNode = true;
                    System.out.println(">>> Found nodes:");
                }

                cow.printNode(currentNodeId);
                currentNodeId = cow.findNextNode(currentNodeId);
            }

            long currentEdgeId = cow.findFirstEdge(currentType);
            while (currentEdgeId != 0)
            {
                if (!hasFoundEdge)
                {
                    hasFoundEdge = true;
                    System.out.println(">>> Found edges:");
                }

                cow.printEdge(currentEdgeId);
                currentEdgeId = cow.findNextEdge(currentEdgeId);
            }

            if (!hasFoundNode && !hasFoundEdge)
            {
                System.out.println(
                    ">>> Could not find a node or edge of type " + currentType
                    + "! Will conclude storage scan.");
                break;
            }

            ++currentType;
        }

        // We're not making any changes to COW, so we can just rollback.
        cow.rollbackTransaction();

        cow.endSession();
    }

    private static void runCowTest()
    {
        CowStorageEngine cow = new CowStorageEngine();

        cow.beginSession();

        // First transaction.
        cow.beginTransaction();

        long nodeId1 = cow.createNode(1, 1, "n1");
        long nodeId2 = cow.createNode(2, 1, "n2");
        long nodeId3 = cow.createNode(3, 2, "n3");
        long nodeId4 = cow.createNode(4, 2, "n4");

        cow.printNode(nodeId1);
        cow.printNode(nodeId2);
        cow.printNode(nodeId3);
        cow.printNode(nodeId4);
        printEmptyLine();

        cow.updateNodePayload(nodeId1, ">n1 v2<");
        cow.printNode(nodeId1);
        printEmptyLine();

        cow.commitTransaction();

        // Second transaction.
        cow.beginTransaction();

        long edgeId1 = cow.createEdge(5, 3, 1, 2, "e5=n1->n2");
        long edgeId2 = cow.createEdge(6, 3, 1, 3, "e6=n1->n3");
        long edgeId3 = cow.createEdge(7, 4, 4, 1, "e7=n4->n1");
        long edgeId4 = cow.createEdge(8, 4, 2, 3, "e8=n2->n3");

        cow.printEdge(edgeId1);
        cow.printEdge(edgeId2);
        cow.printEdge(edgeId3);
        cow.printEdge(edgeId4);
        printEmptyLine();

        cow.updateEdgePayload(edgeId1, ">e5=n1->n2 v2<");
        cow.printEdge(edgeId1);
        printEmptyLine();

        cow.commitTransaction();

        // Third transaction.
        cow.beginTransaction();

        cow.updateNodePayload(nodeId1, ">n1 v3<");

        cow.printNode(nodeId1);
        cow.printNode(nodeId2);
        cow.printNode(nodeId3);
        cow.printNode(nodeId4);
        printEmptyLine();

        cow.updateEdgePayload(edgeId1, ">e5=n1->n2 v3");

        cow.printEdge(edgeId1);
        cow.printEdge(edgeId2);
        cow.printEdge(edgeId3);
        cow.printEdge(edgeId4);
        printEmptyLine();

        cow.commitTransaction();

        // Fourth transaction.
        cow.beginTransaction();

        System.out.println("Iterating through nodes of type 1: ");
        long currentNodeId = cow.findFirstNode(1);
        while (currentNodeId != 0)
        {
            cow.printNode(currentNodeId);
            currentNodeId = cow.findNextNode(currentNodeId);
        }
        printEmptyLine();

        System.out.println("Iterating through edges of type 4: ");
        long currentEdgeId = cow.findFirstEdge(4);
        while (currentEdgeId != 0)
        {
            cow.printEdge(currentEdgeId);
            currentEdgeId = cow.findNextEdge(currentEdgeId);
        }
        printEmptyLine();

        printEmptyLine();
        System.out.println("Deleting edge 5...");
        cow.removeEdge(5);

        System.out.println("Deleting edge 8...");
        cow.removeEdge(8);

        System.out.println("Deleting node 2...");
        cow.removeNode(2);
        printEmptyLine();

        System.out.println("Iterating through nodes of type 1: ");
        currentNodeId = cow.findFirstNode(1);
        while (currentNodeId != 0)
        {
            cow.printNode(currentNodeId);
            currentNodeId = cow.findNextNode(currentNodeId);
        }
        printEmptyLine();

        printEmptyLine();
        System.out.println("Iterating through edges of type 3: ");
        currentEdgeId = cow.findFirstEdge(3);
        while (currentEdgeId != 0)
        {
            cow.printEdge(currentEdgeId);
            currentEdgeId = cow.findNextEdge(currentEdgeId);
        }
        printEmptyLine();

        printEmptyLine();
        System.out.println("Iterating through edges of type 4: ");
        currentEdgeId = cow.findFirstEdge(4);
        while (currentEdgeId != 0)
        {
            cow.printEdge(currentEdgeId);
            currentEdgeId = cow.findNextEdge(currentEdgeId);
        }
        printEmptyLine();

        cow.commitTransaction();

        cow.endSession();

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

        if (indent
            || (getFirstEdgeWithNodeAsFirst(nodeId) == 0 && getFirstEdgeWithNodeAsSecond(nodeId) == 0))
        {
            return;
        }

        for (long edgeId = getFirstEdgeWithNodeAsFirst(nodeId);
            edgeId != 0;
            edgeId = getNextEdgeWithSameFirstNode(edgeId))
        {
            printEdge(edgeId, true);
        }

        for (long edgeId = getFirstEdgeWithNodeAsSecond(nodeId);
            edgeId != 0;
            edgeId = getNextEdgeWithSameSecondNode(edgeId))
        {
            printEdge(edgeId, true);
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

    public native long getFirstEdgeWithNodeAsFirst(long id);
    public native long getFirstEdgeWithNodeAsSecond(long id);

    public native long createEdge(long id, long type, long idFirstNode, long idSecondNode, byte[] payload);
    public native boolean updateEdgePayload(long id, byte[] payload);
    public native boolean removeEdge(long id);

    public native long findFirstEdge(long type);
    public native long findNextEdge(long id);

    public native long getEdgeType(long id);
    public native long getEdgeFirstNode(long id);
    public native long getEdgeSecondNode(long id);
    public native byte[] getEdgePayload(long id);

    public native long getNextEdgeWithSameFirstNode(long id);
    public native long getNextEdgeWithSameSecondNode(long id);
}
