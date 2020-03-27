/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.database;

public class CowStorageEngine
{
    static
    {
        System.loadLibrary("jni_se_mock");
    }
     
    public static void main(String[] args)
    {
        CowStorageEngine cow = new CowStorageEngine();

        cow.initialize(true);

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
        System.out.println("");

        cow.updateNodePayload(nodeId1, ">n1 v2<");
        cow.printNode(nodeId1);
        System.out.println("");

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
        System.out.println("");
        
        cow.updateEdgePayload(edgeId1, ">e5=n1->n2 v2<");
        cow.printEdge(edgeId1);
        System.out.println("");
        
        cow.commitTransaction();
        
        // Third transaction.
        cow.beginTransaction();
        
        cow.updateNodePayload(nodeId1, ">n1 v3<");
        
        cow.printNode(nodeId1);
        cow.printNode(nodeId2);
        cow.printNode(nodeId3);
        cow.printNode(nodeId4);
        System.out.println("");
        
        cow.updateEdgePayload(edgeId1, ">e5=n1->n2 v3");
        
        cow.printEdge(edgeId1);
        cow.printEdge(edgeId2);
        cow.printEdge(edgeId3);
        cow.printEdge(edgeId4);
        System.out.println("");
        
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
        System.out.println("");
        
        System.out.println("Iterating through edges of type 4: ");
        long currentEdgeId = cow.findFirstEdge(4);
        while (currentEdgeId != 0)
        {
            cow.printEdge(currentEdgeId);
            currentEdgeId = cow.findNextEdge(currentEdgeId);
        }
        System.out.println("");
        
        System.out.println("");
        System.out.println("Deleting edge 1...");
        cow.removeEdge(5);
        
        System.out.println("Deleting edge 4...");
        cow.removeEdge(8);
        
        System.out.println("Deleting node 2...");
        cow.removeNode(2);
        System.out.println("");
        
        System.out.println("Iterating through nodes of type 1: ");
        currentNodeId = cow.findFirstNode(1);
        while (currentNodeId != 0)
        {
            cow.printNode(currentNodeId);
            currentNodeId = cow.findNextNode(currentNodeId);
        }
        System.out.println("");
        
        System.out.println("");
        System.out.println("Iterating through edges of type 3: ");
        currentEdgeId = cow.findFirstEdge(3);
        while (currentEdgeId != 0)
        {
            cow.printEdge(currentEdgeId);
            currentEdgeId = cow.findNextEdge(currentEdgeId);
        }
        System.out.println("");
    
        System.out.println("");
        System.out.println("Iterating through edges of type 4: ");
        currentEdgeId = cow.findFirstEdge(4);
        while (currentEdgeId != 0)
        {
            cow.printEdge(currentEdgeId);
            currentEdgeId = cow.findNextEdge(currentEdgeId);
        }
        System.out.println("");
        
        cow.commitTransaction();
        
        System.out.println("All tests passed!");
    }

    public void printEdge(long edgeId)
    {
        printEdge(edgeId, false);
    }
 
    public void printEdge(long edgeId, boolean indent)
    {
        System.out.println("");

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

        System.out.print(" Payload: " + getEdgePayload(edgeId));

        if (!indent) 
        {
            printNode(getEdgeFirstNode(edgeId), true);
            printNode(getEdgeSecondNode(edgeId), true);
            System.out.println("");
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
        System.out.println("");

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

        System.out.print(" Payload: " + getNodePayload(nodeId));

        if (indent
            || (getNextEdgeWithNodeAsFirst(nodeId) == 0 && getNextEdgeWithNodeAsSecond(nodeId) == 0))
        {
            return;
        }

        for (long edgeId = getNextEdgeWithNodeAsFirst(nodeId);
            edgeId != 0;
            edgeId = getNextEdgeWithSameFirstNode(edgeId))
        {
            printEdge(edgeId, true);
        }

        for (long edgeId = getNextEdgeWithNodeAsSecond(nodeId);
            edgeId != 0;
            edgeId = getNextEdgeWithSameSecondNode(edgeId))
        {
            printEdge(edgeId, true);
        }

        System.out.println("");
    }
    
    // Native interface.
    public native void initialize(boolean cleanMemory);

    public native void beginTransaction();
    public native void commitTransaction();
    public native void rollbackTransaction();

    public native long createNode(long id, long type, String payload);
    public native boolean updateNodePayload(long id, String payload);
    public native boolean removeNode(long id);

    public native long findFirstNode(long type);
    public native long findNextNode(long id);

    public native long getNodeType(long id);
    public native String getNodePayload(long id);

    public native long getNextEdgeWithNodeAsFirst(long id);
    public native long getNextEdgeWithNodeAsSecond(long id);

    public native long createEdge(long id, long type, long idFirstNode, long idSecondNode, String payload);
    public native boolean updateEdgePayload(long id, String payload);
    public native boolean removeEdge(long id);

    public native long findFirstEdge(long type);
    public native long findNextEdge(long id);

    public native long getEdgeType(long id);
    public native long getEdgeFirstNode(long id);
    public native long getEdgeSecondNode(long id);
    public native String getEdgePayload(long id);

    public native long getNextEdgeWithSameFirstNode(long id);
    public native long getNextEdgeWithSameSecondNode(long id);
}
