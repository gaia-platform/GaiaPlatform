/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

package com.gaiaplatform.truegraphdb;

public class CowStorageEngine
{
    static
    {
        System.loadLibrary("native_cow_se");
    }
     
    public static void main(String[] args)
    {
        CowStorageEngine cow = new CowStorageEngine();

        cow.initialize();

        // First transaction.
        cow.beginTransaction();

        long idNode1 = cow.createNode(1, 1, "n1");
        long idNode2 = cow.createNode(2, 1, "n2");
        long idNode3 = cow.createNode(3, 2, "n3");
        long idNode4 = cow.createNode(4, 2, "n4");

        cow.printNode(idNode1);
        cow.printNode(idNode2);
        cow.printNode(idNode3);
        cow.printNode(idNode4);
        System.out.println("");

        cow.updateNodePayload(idNode1, ">n1 v2<");
        cow.printNode(idNode1);
        System.out.println("");

        cow.commitTransaction();

        // Second transaction.
        cow.beginTransaction();
        
        long idEdge1 = cow.createEdge(5, 3, 1, 2, "e5=n1->n2");
        long idEdge2 = cow.createEdge(6, 3, 1, 3, "e6=n1->n3");
        long idEdge3 = cow.createEdge(7, 4, 4, 1, "e7=n4->n1");
        long idEdge4 = cow.createEdge(8, 4, 2, 3, "e8=n2->n3");
        
        cow.printEdge(idEdge1);
        cow.printEdge(idEdge2);
        cow.printEdge(idEdge3);
        cow.printEdge(idEdge4);
        System.out.println("");
        
        cow.updateEdgePayload(idEdge1, ">e5=n1->n2 v2<");
        cow.printEdge(idEdge1);
        System.out.println("");
        
        cow.commitTransaction();
        
        // Third transaction.
        cow.beginTransaction();
        
        cow.updateNodePayload(idNode1, ">n1 v3<");
        
        cow.printNode(idNode1);
        cow.printNode(idNode2);
        cow.printNode(idNode3);
        cow.printNode(idNode4);
        System.out.println("");
        
        cow.updateEdgePayload(idEdge1, ">e5=n1->n2 v3");
        
        cow.printEdge(idEdge1);
        cow.printEdge(idEdge2);
        cow.printEdge(idEdge3);
        cow.printEdge(idEdge4);
        System.out.println("");
        
        cow.commitTransaction();
        
        // Fourth transaction.
        cow.beginTransaction();
        
        System.out.println("Iterating through nodes of type 1: ");
        long current = cow.findFirstNode(1);
        while (current != 0)
        {
            cow.printNode(current);
            current = cow.findNextNode(current);
        }
        System.out.println("");
        
        System.out.println("Iterating through edges of type 4: ");
        current = cow.findFirstEdge(4);
        while (current != 0)
        {
            cow.printEdge(current);
            current = cow.findNextEdge(current);
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
        current = cow.findFirstNode(1);
        while (current != 0)
        {
            cow.printNode(current);
            current = cow.findNextNode(current);
        }
        System.out.println("");
        
        System.out.println("");
        System.out.println("Iterating through edges of type 3: ");
        current = cow.findFirstEdge(3);
        while (current != 0)
        {
            cow.printEdge(current);
            current = cow.findNextEdge(current);
        }
        System.out.println("");
    
        System.out.println("");
        System.out.println("Iterating through edges of type 4: ");
        current = cow.findFirstEdge(4);
        while (current != 0)
        {
            cow.printEdge(current);
            current = cow.findNextEdge(current);
        }
        System.out.println("");
        
        cow.commitTransaction();
        
        System.out.println("All tests passed!");
    }
 
    // Native interface.
    private native void initialize();

    private native void beginTransaction();
    private native void commitTransaction();

    private native long createNode(long id, long type, String payload);
    private native void updateNodePayload(long id, String payload);
    private native void removeNode(long id);
    private native void printNode(long id);

    private native long findFirstNode(long type);
    private native long findNextNode(long id);

    private native long getNodeType(long id);
    private native String getNodePayload(long id);

    private native long getNextEdgeWithNodeAsFirst(long id);
    private native long getNextEdgeWithNodeAsSecond(long id);

    private native long createEdge(long id, long type, long idFirstNode, long idSecondNode, String payload);
    private native void updateEdgePayload(long id, String payload);
    private native void removeEdge(long id);
    private native void printEdge(long id);

    private native long findFirstEdge(long type);
    private native long findNextEdge(long id);

    private native long getEdgeType(long id);
    private native long getEdgeFirstNode(long id);
    private native long getEdgeSecondNode(long id);
    private native String getEdgePayload(long id);

    private native long getNextEdgeWithSameFirstNode(long id);
    private native long getNextEdgeWithSameSecondNode(long id);
}
