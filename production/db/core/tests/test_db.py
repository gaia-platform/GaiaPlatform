#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
This module tests the Python wrappers over the low-level Gaia Database API.
"""

# pylint: disable=import-error
from gaia_db_pybind import (
    begin_session,
    end_session,
    begin_transaction,
    commit_transaction,
    print_node,
    gaia_ptr,
    gaia_id,
    gaia_type,
)

# pylint: enable=import-error

# Start session.
begin_session()

# First transaction - dump first 4 nodes (they must exist for catalog objects).
begin_transaction()

node1 = gaia_ptr.from_gaia_id(gaia_id(1))
node2 = gaia_ptr.from_gaia_id(gaia_id(2))
node3 = gaia_ptr.from_gaia_id(gaia_id(3))
node4 = gaia_ptr.from_gaia_id(gaia_id(4))

print_node(node1, False)
print_node(node2, False)
print_node(node3, False)
print_node(node4, False)
print("")

commit_transaction()

# Second transaction - iterate through nodes of type 'database'.
begin_transaction()

print("Iterating through nodes of type 'database' (4294967291): ", flush=True)
node_iter = gaia_ptr.find_first(gaia_type(4294967291))
while not node_iter.is_null():
    print_node(node_iter, False)
    node_iter = node_iter.find_next()
print("")

commit_transaction()

end_session()

print("All tests passed!")
