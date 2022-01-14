---------------------------------------------
-- Copyright (c) 2021 Gaia Platform LLC
--
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
---------------------------------------------

database vlr_example

table parent (
    id uint32 unique,
    children references child[]
)

table child (
    id uint32 unique,
    parent_id uint32,
    parent references parent
        where child.parent_id = parent.id
)
