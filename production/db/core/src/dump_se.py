#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
This module... TBD.
"""

# pylint: disable=import-error
from gaia_db_pybind import (
    begin_session,
    end_session,
    begin_transaction,
    commit_transaction,
    print_node,
    gaia_ptr,
)

# pylint: enable=import-error

begin_session()

begin_transaction()

TYPE_COUNT = 1
IS_DONE = False
while not IS_DONE:
    NO_NODE = False

    print("Iterating through nodes of type: " + str(TYPE_COUNT) + "...")
    node_iter = gaia_ptr.find_first(TYPE_COUNT)

    if node_iter.is_null():
        print("No nodes of type " + str(TYPE_COUNT) + " were found!")
        NO_NODE = True

    while not node_iter.is_null():
        print_node(node_iter, False)
        node_iter = node_iter.find_next()
    print("")

    if NO_NODE:
        print("No nodes of type " + str(TYPE_COUNT) + " were found!")
        print("Terminating database scan!")
        IS_DONE = True

    TYPE_COUNT += 1

commit_transaction()

end_session()
