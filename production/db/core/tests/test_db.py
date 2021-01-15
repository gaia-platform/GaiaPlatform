#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from gaia_db_pybind import *;

# Start session.
begin_session();

# First transaction - dump first 4 nodes (they must exist for catalog objects).
begin_transaction();

node1 = gaia_ptr.open(1);
node2 = gaia_ptr.open(2);
node3 = gaia_ptr.open(3);
node4 = gaia_ptr.open(4);

print_node(node1, False);
print_node(node2, False);
print_node(node3, False);
print_node(node4, False);
print ('');

commit_transaction();

# Second transaction - iterate through nodes of type 'field'.
begin_transaction();

print("Iterating through nodes of type 'database' (4294967291): ");
node_iter = gaia_ptr.find_first(4294967291);
while node_iter.is_null() != True:
    print_node(node_iter, False);
    node_iter = node_iter.find_next();
print ('');

commit_transaction();

end_session();

print ('All tests passed!');
