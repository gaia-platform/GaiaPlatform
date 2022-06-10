////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

/*
 * PostgresSQL "port.h" tries to replace printf() and friends with macros to
 * their own versions. This leads to build error in other headers like spdlog.
 */
#ifdef fprintf
#undef fprintf
#endif

// All Postgres headers and function declarations must have C linkage.
extern "C"
{

// postgres.h must be included prior to other headers.
#include <postgres.h>
// These surrounding comments will ensure that the header order is not changed by clang-format.
#include <catalog/pg_type.h>
#include <lib/stringinfo.h>
#include <nodes/pg_list.h>
#include <utils/array.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>

} // extern "C"

// We use `#pragma GCC` instead of `#pragma clang` for compatibility with both clang and gcc.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmacro-redefined"
#define fprintf fprintf
#pragma GCC diagnostic pop

#include <string>
#include <unordered_map>

#include "gaia/common.hpp"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

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

enum class modify_operation_type_t : int8_t
{
    none = 0,

    insert = 1,
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
    static S* get_state(const char* table_name, size_t expected_field_count)
    {
        // Ensure that the space allocated for S is properly aligned.
        // We'll allocate enough extra space to ensure that the requested buffer
        // can be properly aligned within it.
        size_t state_size = sizeof(S);
        size_t state_alignment = alignof(S);
        size_t buffer_size = state_size + state_alignment - 1;
        void* buffer = palloc0(buffer_size);
        ASSERT_INVARIANT(
            buffer != nullptr,
            "palloc0() failed!");
        void* original_buffer = buffer;
        void* aligned_buffer = std::align(state_alignment, state_size, buffer, buffer_size);
        ASSERT_INVARIANT(
            aligned_buffer >= original_buffer,
            "Aligned pointer should be higher than original pointer!");
        ASSERT_INVARIANT(
            static_cast<uint8_t*>(aligned_buffer) < static_cast<uint8_t*>(original_buffer) + state_alignment,
            "Aligned pointer exceeds original pointer by more than the alignment size!");

        S* state = reinterpret_cast<S*>(aligned_buffer);

        return state->initialize(table_name, expected_field_count) ? state : nullptr;
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
    static std::unordered_map<
        std::string,
        std::pair<gaia::common::gaia_id_t, gaia::common::gaia_type_t>>
        s_map_table_name_to_ids;
};

// A structure holding basic field information.
struct field_information_t
{
    // The position field can hold either a field_position_t or a reference_offset_t value,
    // depending on whether the field is a regular field or a reference field,
    // as indicated by the value of the is_reference field.
    static_assert(sizeof(gaia::common::field_position_t) <= sizeof(uint16_t));
    static_assert(sizeof(gaia::common::reference_offset_t) <= sizeof(uint16_t));
    uint16_t position;

    gaia::common::data_type_t type;

    uint64_t repeated_count;

    bool is_reference;

    // Note: currently, this is used only for the delayed setting of references.
    NullableDatum value_to_set;
};

class state_t
{
protected:
    // state_t is just a base class,
    // so its constructor is protected
    // to prevent the creation of any instances.
    state_t() = default;

    bool initialize(const char* table_name, size_t expected_field_count);

public:
    // Provides the index corresponding to each field.
    // This enables future calls to use index values.
    bool set_field_index(const char* field_name, size_t field_index);

    bool is_gaia_id_field_index(size_t field_index);

    const char* get_table_name();

protected:
    // Store the table name for the convenience of printing it in error messages.
    char* m_table_name;

    // The table id and container id.
    gaia::common::gaia_id_t m_table_id;
    gaia::common::gaia_type_t m_container_id;

    // Count of fields for current table.
    size_t m_field_count;

    // Field information array.
    field_information_t* m_fields;

    // 0-based index of gaia_id attribute in tuple descriptor.
    size_t m_gaia_id_field_index;
};

// The scan state is set up in gaia_begin_foreign_scan,
// is stashed away in node->fdw_private,
// and is fetched in gaia_iterate_foreign_scan and gaia_rescan_foreign_scan.
class scan_state_t : public state_t
{
    friend class adapter_t;

protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    scan_state_t(const scan_state_t&) = delete;
    scan_state_t& operator=(const scan_state_t&) = delete;

    // Only adapter_t can create instances of scan_state_t.
    scan_state_t();

public:
    // Scan API.
    bool initialize_scan();
    bool has_scan_ended();
    NullableDatum extract_field_value(size_t field_index);
    bool scan_forward();

protected:
    // The iterator that we are using for the current scan.
    gaia::common::iterators::generator_iterator_t<gaia::db::gaia_ptr_t> m_iterator;

    // Pointer to the deserialized payload of the current record.
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
    modify_state_t();

    bool modify_record(uint64_t gaia_id, modify_operation_type_t modify_operation_type);

public:
    // Modify API.
    void initialize_payload();
    void set_field_value(size_t field_index, const NullableDatum& field_value);
    bool insert_record(uint64_t gaia_id);
    bool update_record(uint64_t gaia_id);
    bool delete_record(uint64_t gaia_id);
    void end_modify();

protected:
    // Because a vector manages its own memory
    // and state objects are not deallocated normally by Postgres,
    // we need to allocate the vector dynamically,
    // so we can release its memory manually in end_modify().
    std::vector<uint8_t>* m_current_payload;

    // Direct pointer to the binary_schema stored in the catalog. This is safe
    // to hold around in our scenario because FDW adapter does not modify the
    // binary schema, and the memory is managed by the gaia_db_server.
    const uint8_t* m_binary_schema;
    size_t m_binary_schema_size;
};

} // namespace fdw
} // namespace gaia
