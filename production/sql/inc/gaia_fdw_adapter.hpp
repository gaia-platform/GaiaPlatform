/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>

// All Postgres headers and function declarations must have C linkage.
extern "C"
{

#include "postgres.h"

// postgres.h must be included prior to these headers.
#include "catalog/pg_type.h"
#include "nodes/pg_list.h"
#include "utils/builtins.h"

} // extern "C"

#include "gaia_common.hpp"
#include "gaia_ptr.hpp"
#include "retail_assert.hpp"

namespace gaia
{
namespace fdw
{

typedef void (*option_handler_fn)(const char* option_name, const char* option_value, Oid context_id);

// Describes the valid options for objects that use this wrapper.
struct option_t
{
    const char* name;

    // Oid of catalog in which option may appear.
    Oid context_id;

    option_handler_fn handler;
};

// Check if the provided option is one of the valid options.
// context_id is the Oid of the catalog holding the object the option is for.
// If handler is registered for this option, invoke it.
bool validate_and_apply_option(const char* option_name, const char* option_value, Oid context_id);

void append_context_option_names(Oid context_id, StringInfoData& string_data);

enum class edit_state_t : int8_t
{
    none = 0,

    create = 1,
    update = 2,
};

// The adapter_t class provides the interface through which the FDW
// can interact with the database.
// Instances of scan_state_t will be used during scan operations
// and instances of modify_state_t will be used for insert/update/delete operations.
class adapter_t
{
protected:
    // adapter_t is just a container for static methods,
    // so its constructor is protected
    // to prevent the creation of any instances.
    adapter_t() = default;

public:
    static void begin_session();
    static void end_session();

    static bool is_transaction_open();
    static bool begin_transaction();
    static bool commit_transaction();

    static bool is_gaia_id_name(const char* name);

    static uint64_t get_new_gaia_id();

    static List* get_ddl_command_list(const char* server_name);

    static bool get_ids(
        const char* table_name,
        gaia::common::gaia_id_t& table_id,
        gaia::common::gaia_type_t& container_id);

    template <class S>
    static S* get_state(const char* table_name, size_t count_fields)
    {
        S* state = (S*)palloc0(sizeof(S));

        return state->initialize(table_name, count_fields) ? state : nullptr;
    }

protected:
    static void initialize_caches();

protected:
    // HACKHACK: global counter to simulate nested transactions. Because a DELETE
    // plan is nested within a scan, committing the write txn will invalidate the
    // read txn. We get around this by using a refcount to track the txn nesting
    // state, so we only open a txn when the counter is initially incremented from 0
    // and only commit a txn when the counter is decremented to 0. An unsynchronized
    // global counter is ok since there's no concurrency within a postgres backend.
    //
    // Use signed int so we can assert it is non-negative.
    static int s_txn_reference_count;

    // Small cache to enable looking up a table type by name.
    static std::unordered_map<std::string, std::pair<gaia::common::gaia_id_t, gaia::common::gaia_type_t>> s_map_table_name_to_ids;
};

// A structure holding basic field information.
struct field_information_t
{
    gaia::common::field_position_t position;
    gaia::common::data_type_t type;

    bool is_reference()
    {
        return type == data_type_t::e_references;
    }
};

class state_t
{
protected:
    // state_t is just a base class,
    // so its constructor is protected
    // to prevent the creation of any instances.
    state_t() = default;

    bool initialize(const char* table_name, size_t count_fields);

public:
    // Provides the index corresponding to each field.
    // This enables future calls to use index values.
    bool set_field_index(const char* field_name, size_t field_index);

    bool is_gaia_id_field_index(size_t field_index);

protected:
    // The table id and container id.
    gaia::common::gaia_id_t m_table_id;
    gaia::common::gaia_type_t m_container_id;

    // Count of fields for current table.
    size_t m_count_fields;

    // Field information array.
    field_information_t* m_fields;

    // 0-based index of gaia_id attribute in tuple descriptor.
    size_t m_gaia_id_field_index;
};

// The scan state is set up in gaia_begin_foreign_scan,
// is stashed away in node->fdw_private,
// and is fetched in gaia_iterate_foreign_scan.
class scan_state_t : public state_t
{
    friend class adapter_t;

protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    scan_state_t(const scan_state_t&) = delete;
    scan_state_t& operator=(const scan_state_t&) = delete;

    // Only adapter_t can create instances of scan_state_t.
    scan_state_t() = default;

public:
    // Scan API.
    bool initialize_scan();
    bool has_scan_ended();
    Datum extract_field_value(size_t field_index);
    bool scan_forward();

protected:
    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr m_current_node;

    // Pointer to the deserialized payload of the current_node.
    const uint8_t* m_current_payload;
};

// The modify state is for maintaining state of modify operations.
//
// It is set up in gaiaBeginForeignModify, stashed in rinfo->ri_FdwState,
// and is subsequently used in gaiaExecForeignInsert, gaiaExecForeignUpdate,
// gaiaExecForeignDelete, and gaiaEndForeignModify.
class modify_state_t : public state_t
{
    friend class adapter_t;

protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    modify_state_t(const modify_state_t&) = delete;
    modify_state_t& operator=(const modify_state_t&) = delete;

    // Only adapter_t can create instances of modify_state_t.
    modify_state_t() = default;

    bool initialize(const char* table_name, size_t count_fields);

    bool edit_record(uint64_t gaia_id, edit_state_t edit_state);

public:
    // Modify API.
    void set_field_value(size_t field_index, const Datum& field_value);
    bool insert_record(uint64_t gaia_id);
    bool update_record(uint64_t gaia_id);
    bool delete_record(uint64_t gaia_id);

protected:
    void copy_payload(const std::vector<uint8_t>& payload);

protected:
    uint8_t* m_current_payload;
    size_t m_current_payload_size;

    const uint8_t* m_binary_schema;
    size_t m_binary_schema_size;
};

} // namespace fdw
} // namespace gaia
