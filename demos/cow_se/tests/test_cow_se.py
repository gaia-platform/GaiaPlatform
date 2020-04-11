#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from cow_se import *;

gaia_mem_base.init(True);

# First transaction.
begin_transaction();

node1 = create_node(1, 1, "n1");
node2 = create_node(2, 1, "n2");
node3 = create_node(3, 2, "n3");
node4 = create_node(4, 2, "n4");

print_node(node1, False);
print_node(node2, False);
print_node(node3, False);
print_node(node4, False);
print ('');

update_node(node1, ">n1 v2<");
print_node(node1, False);
print ('');

commit_transaction();

# Second transaction.
begin_transaction();

edge1 = create_edge(5, 3, 1, 2, "e5=n1->n2");
edge2 = create_edge(6, 3, 1, 3, "e6=n1->n3");
edge3 = create_edge(7, 4, 4, 1, "e7=n4->n1");
edge4 = create_edge(8, 4, 2, 3, "e8=n2->n3");

print_edge(edge1, False);
print_edge(edge2, False);
print_edge(edge3, False);
print_edge(edge4, False);
print ('');

update_edge(edge1, ">e5=n1->n2 v2<");
print_edge(edge1, False);
print ('');

commit_transaction();

# Third transaction.
begin_transaction();

node1 = gaia_se_node.open(1);
update_node(node1, ">n1 v3<");

node2 = gaia_se_node.open(2);
node3 = gaia_se_node.open(3);
node4 = gaia_se_node.open(4);

print_node(node1, False);
print_node(node2, False);
print_node(node3, False);
print_node(node4, False);
print ('');

edge1 = gaia_se_edge.open(5);
update_edge(edge1, ">e5=n1->n2 v3<");

edge2 = gaia_se_edge.open(6);
edge3 = gaia_se_edge.open(7);
edge4 = gaia_se_edge.open(8);

print_edge(edge1, False);
print_edge(edge2, False);
print_edge(edge3, False);
print_edge(edge4, False);
print ('');

commit_transaction();

# Fourth transaction.
begin_transaction();

print("Iterating through nodes of type 1: ");
node_iter = gaia_se_node_ptr.find_first(1);
while node_iter.get() != None:
    print_node(node_iter, False);
    node_iter = node_iter.find_next();
print ('');

print("Iterating through edges of type 4: ");
edge_iter = gaia_se_edge_ptr.find_first(4);
while edge_iter.get() != None:
    print_edge(edge_iter, False);
    edge_iter = edge_iter.find_next();
print ('');

print ('');
print("Deleting edge 5...");
edge1 = gaia_se_edge.open(5);
gaia_se_edge_ptr.remove(edge1);

print("Deleting edge 8...");
edge4 = gaia_se_edge.open(8);
gaia_se_edge_ptr.remove(edge4);

print("Deleting node 2...");
node2 = gaia_se_node.open(2);
gaia_se_node_ptr.remove(node2);
print ('');

print("Iterating through nodes of type 1: ");
node_iter = gaia_se_node_ptr.find_first(1);
while node_iter.get() != None:
    print_node(node_iter, False);
    node_iter = node_iter.find_next();
print ('');

print ('');
print("Iterating through edges of type 3: ");
edge_iter = gaia_se_edge_ptr.find_first(3);
while edge_iter.get() != None:
    print_edge(edge_iter, False);
    edge_iter = edge_iter.find_next();
print ('');

print ('');
print("Iterating through edges of type 4: ");
edge_iter = gaia_se_edge_ptr.find_first(4);
while edge_iter.get() != None:
    print_edge(edge_iter, False);
    edge_iter = edge_iter.find_next();
print ('');

commit_transaction();

print ('All tests passed!');
