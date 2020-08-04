/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

// All Postgres headers and function declarations must have C linkage.
extern "C" {

#include "postgres.h"

#include "nodes/pg_list.h"

} // extern "C"


#include "airport_demo_type_mapping.hpp"
#include "system_catalog_type_mapping.hpp"

namespace gaia
{
namespace fdw
{

const char* const c_gaia_id = "gaia_id";

enum class edit_state_t: int8_t
{
    none = 0,

    create = 1,
    update = 2,
};

class gaia_fdw_scan_state_t;
class gaia_fdw_modify_state_t;
class gaia_fdw_adapter_t
{
    // For providing access to get_mapping internal helper.
    friend class gaia_fdw_scan_state_t;
    friend class gaia_fdw_modify_state_t;

protected:

    // gaia_fdw_adapter_t is just a container for static methods,
    // so its constructor is protected
    // to prevent the creation of any instances.
    gaia_fdw_adapter_t() = default;

public:

    static bool is_transaction_open();
    static bool begin_transaction();
    static bool commit_transaction();

    static uint64_t get_new_gaia_id();

    static List* get_ddl_command_list(const char* server_name);

    template <class S>
    static S* get_state(
        const char* table_name, size_t count_accessors);

protected:

    // HACKHACK: global counter to simulate nested transactions. Because a DELETE
    // plan is nested within a scan, committing the write txn will invalidate the
    // read txn. We get around this by using a refcount to track the txn nesting
    // state, so we only open a txn when the counter is initially incremented from 0
    // and only commit a txn when the counter is decremented to 0. An unsynchronized
    // global counter is ok since there's no concurrency within a postgres backend.
    //
    // Use signed int so we can assert it is non-negative.
    static int s_transaction_reference_count;
};

class gaia_fdw_state_t
{
protected:

    // gaia_fdw_state_t is just a base class,
    // so its constructor is protected
    // to prevent the creation of any instances.
    gaia_fdw_state_t() = default;

    bool initialize(const char* table_name, size_t count_accessors);

protected:

    const relation_attribute_mapping_t* m_mapping;
};


// The scan state is set up in gaia_begin_foreign_scan,
// is stashed away in node->fdw_private,
// and is fetched in gaia_iterate_foreign_scan.
class gaia_fdw_scan_state_t : public gaia_fdw_state_t
{
    friend class gaia_fdw_adapter_t;

protected:

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    gaia_fdw_scan_state_t(const gaia_fdw_scan_state_t&) = delete;
    gaia_fdw_scan_state_t& operator=(const gaia_fdw_scan_state_t&) = delete;

    // Only gaia_fdw_adapter_t can create instances of gaia_fdw_scan_state_t.
    gaia_fdw_scan_state_t() = default;

    bool initialize(const char* table_name, size_t count_accessors);
    void deserialize_record();

public:

    bool set_accessor_index(const char* accessor_name, size_t accessor_index);

    bool initialize_scan();
    bool has_scan_ended();
    Datum extract_field_value(size_t field_index);
    bool scan_forward();

protected:

    root_object_deserializer_fn m_deserializer;

    // flatbuffer accessor functions indexed by attrnum.
    attribute_accessor_fn* m_indexed_accessors;

    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr m_current_node;

    // Pointer to the deserialized payload of the current_node.
    const void* m_current_object_root;
};

// The modify state is for maintaining state of modify operations.
//
// It is set up in gaiaBeginForeignModify, stashed in rinfo->ri_FdwState,
// and is subsequently used in gaiaExecForeignInsert, gaiaExecForeignUpdate,
// gaiaExecForeignDelete, and gaiaEndForeignModify.
class gaia_fdw_modify_state_t : public gaia_fdw_state_t
{
    friend class gaia_fdw_adapter_t;

protected:

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    gaia_fdw_modify_state_t(const gaia_fdw_modify_state_t&) = delete;
    gaia_fdw_modify_state_t& operator=(const gaia_fdw_modify_state_t&) = delete;

    // Only gaia_fdw_adapter_t can create instances of gaia_fdw_modify_state_t.
    gaia_fdw_modify_state_t() = default;

    bool initialize(const char* table_name, size_t count_accessors);

    bool edit_record(uint64_t gaia_id, edit_state_t edit_state);

public:

    bool set_builder_index(const char* builder_name, size_t builder_index);

    void initialize_modify();
    bool is_gaia_id_field_index(size_t field_index);
    void set_field_value(size_t field_index, const Datum& field_value);
    bool insert_record(uint64_t gaia_id);
    bool update_record(uint64_t gaia_id);
    bool delete_record(uint64_t gaia_id);
    void finalize_modify();

protected:

    builder_initializer_fn m_initializer;
    builder_finalizer_fn m_finalizer;

    // flatbuffer attribute builder functions indexed by attrnum.
    attribute_builder_fn* m_indexed_builders;

    // flatbuffers builder for INSERT and UPDATE.
    flatcc_builder_t m_builder;

    // Tracks whether the builder has been initialized.
    bool m_has_initialized_builder;

    // 0-based index of gaia_id attribute in tuple descriptor.
    int m_pk_attr_idx;

    gaia_type_t m_gaia_type_id;
};

}
}
