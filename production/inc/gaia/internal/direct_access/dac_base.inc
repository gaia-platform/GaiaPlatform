////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

/**
 * Used by iterator class to maintain state of an iteration.
 *
 * This is needed to ensure proper destruction of derived instances via the virtual destructor.
 */
struct dac_base_iterator_state_t
{
    virtual ~dac_base_iterator_state_t() = default;
};

/**
 * Used by edc object, writer, and iterator classes.
 * Not for use outside the context of those classes.
 */
class dac_db_t
{
    friend class dac_base_reference_t;

protected:
    // Low-level interface for iterating over objects of a given container.
    static std::shared_ptr<dac_base_iterator_state_t> initialize_iterator(common::gaia_type_t container_type_id);
    static common::gaia_id_t get_iterator_value(std::shared_ptr<dac_base_iterator_state_t> iterator_state);
    static bool advance_iterator(std::shared_ptr<dac_base_iterator_state_t> iterator_state);

    static common::gaia_id_t get_reference(common::gaia_id_t id, common::reference_offset_t slot);
    static common::gaia_id_t insert(common::gaia_type_t container, size_t data_size, const void* data);
    static void update(common::gaia_id_t id, size_t data_size, const void* data);
    static void delete_row(common::gaia_id_t id, bool force = false);
    static bool get_type(common::gaia_id_t id, common::gaia_type_t& type);

    static bool insert_into_reference_container(common::gaia_id_t parent_id, common::gaia_id_t id, common::reference_offset_t anchor_slot);
    static bool remove_from_reference_container(common::gaia_id_t parent_id, common::gaia_id_t id, common::reference_offset_t anchor_slot);
};

void report_invalid_object_id(common::gaia_id_t id);
void report_invalid_object_type(
    common::gaia_id_t id,
    common::gaia_type_t expected_type,
    const char* expected_typename,
    common::gaia_type_t actual_type);
void report_invalid_object_state();
void report_invalid_object_state(
    common::gaia_id_t parent_id,
    common::gaia_id_t chile_id,
    const char* child_type);
