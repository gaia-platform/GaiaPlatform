# The Catalog Bootstrap Process

Core catalog types are `gaia_database`, `gaia_table`, `gaia_field`, `gaia_relationship` and `gaia_index`. Currently, `gaia_index` is not used during the process of bootstrapping the catalog. However, it may be used during DDL processing because a scheme defines an index.

## `initialize_catalog()`

The catalog initialization function is called by `gaiac` and `gaia::system::initialize()`.

`initialize_catalog()` creates a singleton `ddl_executor_t` object, which calls `boostrap_catalog()` in its constructor. The `ddl_executor_t` object contains (among others) the public methods `create_database()`, `create_table()`, `create_relationships()` and `create_index()`. Private methods, used in `bootstrap_catalog()`, are also in the object, named `create_table_impl()` and `create_index()`. Each of these methods will call the respective DAC `insert_row()` method to create the `gaia_database`, `gaia_table`, `gaia_relationship` or `gaia_index` rows constituting the catalog, which is shown in today's form (January 2022) below:

| id | type | name |
| -- | ---- | ---- |
| 0000001 | gaia_database | name |
| 0000002 | gaia_table | gaia_database |
| 0000003 | gaia_field | name |
| 0000004 | gaia_table | gaia_table |
| 0000005 | gaia_field | name |
| 0000006 | gaia_field | type |
| 0000007 | gaia_field | is_system |
| 0000008 | gaia_field | binary_schema |
| 0000009 | gaia_field | serialization_template |
| 000000a | gaia_relationship | gaia_catalog_database_table (gaia_database -> gaia_table) |
| 000000b | gaia_table | gaia_field |
| 000000c | gaia_field | name |
| 000000d | gaia_field | type |
| 000000e | gaia_field | repeated_count |
| 000000f | gaia_field | position |
| 0000010 | gaia_field | deprecated |
| 0000011 | gaia_field | active |
| 0000012 | gaia_field | unique |
| 0000013 | gaia_relationship | gaia_catalog_table_field (gaia_table -> gaia_field) |
| 0000014 | gaia_table | gaia_relationship |
| 0000015 | gaia_field | name |
| 0000016 | gaia_field | to_parent_link_name |
| 0000017 | gaia_field | to_child_link_name |
| 0000018 | gaia_field | cardinality |
| 0000019 | gaia_field | parent_required |
| 000001a | gaia_field | deprecated |
| 000001b | gaia_field | first_child_offset |
| 000001c | gaia_field | next_child_offset |
| 000001d | gaia_field | prev_child_offset |
| 000001e | gaia_field | parent_offset |
| 000001f | gaia_field | parent_field_positions |
| 0000020 | gaia_field | child_field_positions |
| 0000021 | gaia_relationship | gaia_catalog_relationship_parent (gaia_table -> gaia_relationship) |
| 0000022 | gaia_relationship | gaia_catalog_relationship_child (gaia_table -> gaia_relationship) |
| 0000023 | gaia_table | gaia_ruleset |
| 0000024 | gaia_field | name |
| 0000025 | gaia_field | active_on_startup |
| 0000026 | gaia_field | table_ids |
| 0000027 | gaia_field | source_location |
| 0000028 | gaia_field | serial_stream |
| 0000029 | gaia_table | gaia_rule |
| 000002a | gaia_field | name |
| 000002b | gaia_relationship | gaia_catalog_ruleset_rule (gaia_ruleset -> gaia_rule) |
| 000002c | gaia_table | gaiaindex |
| 000002d | gaia_field | name |
| 000002e | gaia_field | unique |
| 000002f | gaia_field | type |
| 0000030 | gaia_field | fields |
| 0000031 | gaia_relationship | gaia_catalog_table_index (gaia_table -> gaia_index) |
| 0000032 | gaia_database | event_log |
| 0000033 | gaia_table | event_log |
| 0000034 | gaia_field | event_type |
| 0000035 | gaia_field | type_id |
| 0000036 | gaia_field | record_id |
| 0000037 | gaia_field | column_id |
| 0000038 | gaia_field | timestamp |
| 0000039 | gaia_field | rules_invoked |
| 000003a | gaia_database | (empty database) |

The DAC definitions for the core catalog types are in `gaia_catalog.h`, which is stored in the project and usable by any procedural code that performs catalog manipulation.

Since the core catalog types are needed to represent their definitions, it becomes a chicken-and-egg problem to create them in the first place. This was initially done by compiling a `catalog.ddl` file to create the definitions used in `bootstrap_catalog()`.

Today, the procedure to modify `gaia_catalog.h` is as follows:

- Modify the sequence of `create_` calls in `bootstrap_catalog()` to correspond to the new definition of the catalog.
- Rebuild `gaiac` only.
- Create the project files using the new `gaiac` as follows:
  - `gaia_db_server --persistence disabled &`
  - `gaiac --generate --db-name catalog`
  - `cp catalog_generated.h $GAIA_REPO/production/inc/gaia_internal/catalog/catalog_generated.h`
  - `cp gaia_catalog.cpp $GAIA_REPO/production/catalog/src/gaia_catalog.cpp`
  - `cp gaia_catalog.h $GAIA_REPO/production/inc/gaia_internal/catalog/gaia_catalog.h`
- Adjust any usages of catalog `insert_row()` methods to correspond to the changes in `bootstrap_catalog()`.
- Rebuild everything.

## Catalog Core

The constants defined in `catalog_core.hpp` are copied from `gaia_catalog.h`. They are offsets into the `references` array stored in the row's object. The naming convention is:
```
c_<record-type-name><role><relationship-name>
```
where \<role\> is `first`, `next` or `parent`. When \<record-type\> is the parent in a relationship, it will contain a first role. When it is the child in a relationship, it will contain the parent and next roles. The \<relationship-name\> matches the relationship name defined in the DDL.

Whenever the generated catalog header `gaia_catalog.h` has new or different values for any of these constants, or if any are added, the list in `catalog_core.hpp` must be updated to parallel them.

These constants are used in the file type_metadata.cpp to define type metadata used during the bootstrap to obtain the number of slots required by the objects used to store rows of each type. Every relationship in which a type is the parent will require one slot in its references array. Every relationship in which it is the child will require two slots.

For each catalog type, there must exist a `value_type` defined like:
```
auto gaia_database = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_database);
```
where `catalog_type_type::gaia_database` is defined in `system_table_types.hpp`.

Each of these `value_type` values is used to define it's role in relationships by including them in `relationship_t` objects which are in turn added into the metadata cache.

### Used by FDW
`catalog_core_t` contains definitions necessary to query the catalog metadata. It maintains an alternate representation synthesized from the catalog. The header `catalog_core.hpp` contains definitions for `field_view_t`, `table_view_t`, `relationship_view_t` and `index_view_t`. Each of these should contain field definitions that parallel the definitions created in `bootstrap_catalog()`, except that only the fields needed to define catalog rows are required to be present. For every field defined in the `*_view_t` definitions in `catalog_core.hpp`, a fetcher function is defined in `catalog_core.cpp`.

## Type Registry

### Adding a relationship

`type_registry_t::init()` (in `type_metadata.cpp`) is called during the first `gaia_database_t::insert_row()` call within `create_database()`. The `relationship_t` structure is defined in `inc/gaia_internal/db/gaia_relationships.hpp`.

For every relationship named RELM (parent) and REL1 (child), between TYPEA_t and TYPEB_t, modeled in the DDL like this:
```
table TYPEA (
  ...
  RELM references TYPEB[]
)
...
table TYPEB (
  ...
  REL1 references TYPEA
)
```
the following definitions must be included:
```
auto TYPEA = static_cast<gaia_type_t::value_type>(system_table_type_t::TYPEA);
auto TYPEB = static_cast<gaia_type_t::value_type>(system_table_type_t::TYPEB);
 ...
auto TYPEA_TYPEB_relationship = std::make_shared<relationship_t>(relationship_t{
  .parent_type = TYPEA,
  .child_type = TYPEB,
  .first_child_offset = catalog_core_t::c_TYPEA_first_RELMs,
  .next_child_offset = catalog_core_t::c_TYPEB_next_REL1,
  .prev_child_offset = c_invalid_reference_offset,
  .parent_offset = catalog_core_t::c_TYPEB_parent_REL1,
  .cardinality = cardinality_t::many,
  .parent_required = false,
  .value_linked = false});
```
NOTES:
`type_registry_t` owns `m_metadata_registry`, which maps `gaia_type_t` to one instance of a `type_metadata_t`. This is populated by `gaia_database_t::insert_row()` for each core catalog type.


`type_registry_t::create(gaia_type_t type)` is called during creation of a non-catalog row. It obtains the metadata for the record type from the catalog.



QUESTIONS:
Why is `create_database()` followed by `create_table_impl(..., "gaia_database", ...)`? The `create_database()` creates a `gaia_database_t` row as gaia_id_t #1. The `create_table_impl` creates the `gaia_table_t` and `gaia_field_t` rows that define the `gaia_database_t`. As two database rows, #2 and #3, they "define" the database row #1.

In db/core/src/flatbuffers, there is a list of flatbuffer schemas:

- `gaia_field.fbs`
- `gaia_index.fbs`
- `gaia_relationship.fbs`
- `gaia_table.fbs`
- `messages.fbs`

It seems they should correspond with the `*_view_t` definitions in `catalog_core.hpp`, but they don't. When, how and why are they used? Their generated headers are included in catalog_core.cpp. They are used by the fetcher methods, e.g.
```
field_position_t field_view_t::position() const
{
    return catalog::Getgaia_field(m_obj_ptr->data())->position();
}
```

This uses `Getgaia_field()` which is defined in `gaia_field_generated.h`.

These methods are not called during `gaiac` or `test_catalog_types`. Are they needed by the server? Only be running system after all bootstrapping/loading done? They may only be used by the FDW.

However, the `catalog_core.hpp` constants are used in `type_metadata.cpp`.