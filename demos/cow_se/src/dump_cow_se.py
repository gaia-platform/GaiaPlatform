#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from cow_se import *;

gaia_mem_base.init(False);

begin_transaction();

type = 1;
done = False;
while not(done):
    no_node = False;
    no_edge = False;

    print("Iterating through nodes of type: " + str(type) + "...");
    node_iter = gaia_se_node_ptr.find_first(type);

    if (node_iter.get() == None):
        print("No nodes of type " + str(type) + " were found!");
        no_node = True;

    while node_iter.get() != None:
        print_node(node_iter, False);
        node_iter = node_iter.find_next();
    print ("");

    print("Iterating through edges of type " + str(type) + "...");
    edge_iter = gaia_se_edge_ptr.find_first(type);

    if (edge_iter.get() == None):
        print("No edges of type " + str(type) + " were found!");
        no_edge = True;

    while edge_iter.get() != None:
        print_edge(edge_iter, False);
        edge_iter = edge_iter.find_next();
    print ("===");

    if (no_node and no_edge):
        print("No nodes or edges of type " + str(type) + " were found!");
        print("Terminating graph scan!");
        done = True;

    type = type + 1;

commit_transaction();
