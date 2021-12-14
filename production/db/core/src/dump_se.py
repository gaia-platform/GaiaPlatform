#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

from gaia_db_pybind import *

begin_session()

begin_transaction()

type = 1
done = False
while not done:
    no_node = False

    print("Iterating through nodes of type: " + str(type) + "...")
    node_iter = gaia_ptr.find_first(type)

    if node_iter.is_null():
        print("No nodes of type " + str(type) + " were found!")
        no_node = True

    while not node_iter.is_null():
        print_node(node_iter, False)
        node_iter = node_iter.find_next()
    print ("")

    if no_node:
        print("No nodes of type " + str(type) + " were found!")
        print("Terminating database scan!")
        done = True

    type += 1

commit_transaction()

end_session()
