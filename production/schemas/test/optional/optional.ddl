---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database optional_sandbox;

table optional_values
(
    -- Optional fields.
    optional_uint8 uint8 optional,
    optional_uint16 uint16 optional,
    optional_uint32 uint32 optional,
    optional_uint64 uint64 optional,
    optional_int8 int8 optional,
    optional_int16 int16 optional,
    optional_int32 int32 optional,
    optional_int64 int64 optional,
    optional_float float optional,
    optional_double double optional,
    optional_bool bool optional,
    -- Mix non-optional fields.
    non_optional_int uint32,
    non_optional_fp double,
    non_optional_bool bool
)

table optional_insert_override
(
    optional_uint8 uint8 optional,
    optional_double double optional,
    non_optional_bool bool
)

table optional_vlr_parent
(
    id uint32 unique optional,
    child references optional_vlr_child[]
)

table optional_vlr_child
(
    parent_id uint32 optional,
    parent references optional_vlr_parent
        where optional_vlr_child.parent_id = optional_vlr_parent.id
)
