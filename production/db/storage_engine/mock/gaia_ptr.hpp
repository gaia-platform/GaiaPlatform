/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_hash_map.hpp"
#include "storage_engine_client.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
struct gaia_se_node;
struct gaia_se_edge;

template <typename T>
class gaia_ptr
{
    // These two structs need to access the gaia_ptr protected constructors.
    friend struct gaia_se_node;
    friend struct gaia_se_edge;

public:
    gaia_ptr (const std::nullptr_t = nullptr)
        :row_id(0) {}

    gaia_ptr (const gaia_ptr& other)
        :row_id (other.row_id) {}

    operator T* () const
    {
        return to_ptr();
    }

    T& operator * () const
    {
        return *to_ptr();
    }

    T* operator -> () const
    {
        return to_ptr();
    }

    bool operator == (const gaia_ptr<T>& other) const
    {
        return row_id == other.row_id;
    }

    bool operator == (const std::nullptr_t) const
    {
        return to_ptr() == nullptr;
    }

    bool operator != (const std::nullptr_t) const
    {
        return to_ptr() != nullptr;
    }

    operator bool () const
    {
        return to_ptr() != nullptr;
    }

    const T* get() const
    {
        return to_ptr();
    }

    gaia_ptr<T>& clone()
    {
        auto old_this = to_ptr();
        auto old_offset = to_offset();
        auto new_size = sizeof(T) + old_this->payload_size;

        allocate(new_size);
        auto new_this = to_ptr();

        memcpy (new_this, old_this, new_size);

        client::tx_log (row_id, old_offset, to_offset());

        return *this;
    }

    gaia_ptr<T>& update_payload(size_t payload_size, const void* payload)
    {
        auto old_this = to_ptr();
        auto old_offset = to_offset();

        allocate(sizeof(T) + payload_size);

        auto new_this = to_ptr();
        auto new_payload = &new_this->payload;

        memcpy (new_this, old_this, sizeof(T));
        new_this->payload_size = payload_size;
        memcpy (new_payload, payload, payload_size);

        client::tx_log (row_id, old_offset, to_offset());

        return *this;
    }

    static gaia_ptr<T> find_first(gaia_type_t type)
    {
        gaia_ptr<T> ptr;
        ptr.row_id = 1;

        if (!ptr.is(type))
        {
            ptr.find_next(type);
        }

        return ptr;
    }

    gaia_ptr<T> find_next()
    {
        if (gaia_ptr<T>::row_id)
        {
            find_next(gaia_ptr<T>::to_ptr()->type);
        }

        return *this;
    }

    gaia_ptr<T> operator++()
    {
        if (gaia_ptr<T>::row_id)
        {
            find_next(gaia_ptr<T>::to_ptr()->type);
        }
        return *this;
    }

    bool is_null() const
    {
        return row_id == 0;
    }

    int64_t get_id() const
    {
        return to_ptr()->id;
    }

    static void remove (gaia_ptr<T>&);

protected:
    gaia_ptr (const gaia_id_t id, bool is_edge = false)
    {
        gaia_id_t id_copy = preprocess_id(id, is_edge);

        row_id = gaia_hash_map::find(id_copy);
    }

    gaia_ptr (const gaia_id_t id, const size_t size, bool is_edge = false, bool log_updates = true)
        :row_id(0)
    {
        gaia_id_t id_copy = preprocess_id(id, is_edge);

        se_base::hash_node* hash_node = gaia_hash_map::insert (id_copy);
        hash_node->row_id = row_id = client::allocate_row_id();
        client::allocate_object(row_id, size);

        // writing to log will be skipped for recovery
        if (log_updates) {
            client::tx_log (row_id, 0, to_offset());
        }
    }

    gaia_id_t preprocess_id(const gaia_id_t& id, bool is_edge = false)
    {
        check_id(id);

        gaia_id_t id_copy = id;
        if (is_edge)
        {
            id_copy = id_copy | c_edge_flag;
        }
        return id_copy;
    }

    void allocate (const size_t size)
    {
        client::allocate_object(row_id, size);
    }

    T* to_ptr() const
    {
        client::verify_tx_active();

        return row_id && (*client::s_offsets)[row_id]
            ? (T*)(client::s_data->objects + (*client::s_offsets)[row_id])
            : nullptr;
    }

    int64_t to_offset() const
    {
        client::verify_tx_active();

        return row_id
            ? (*client::s_offsets)[row_id]
            : 0;
    }

    bool is (gaia_type_t type) const
    {
        return to_ptr() && to_ptr()->type == type;
    }

    void find_next(gaia_type_t type)
    {
        // search for rows of this type within the range of used slots
        while (++row_id && row_id < client::s_data->row_id_count+1)
        {
            if (is (type))
            {
                return;
            }
        }
        row_id = 0;
    }

    void reset()
    {
        client::tx_log (row_id, to_offset(), 0);
        (*client::s_offsets)[row_id] = 0;
        row_id = 0;
    }

    int64_t row_id;
};

struct gaia_se_node
{
public:
    gaia_ptr<gaia_se_edge> next_edge_first;
    gaia_ptr<gaia_se_edge> next_edge_second;

    gaia_id_t id;
    gaia_type_t type;
    size_t payload_size;
    char payload[0];

    static gaia_id_t generate_id()
    {
        return se_base::generate_id();
    }

    static gaia_ptr<gaia_se_node> create (
        gaia_id_t id,
        gaia_type_t type,
        size_t payload_size,
        const void* payload,
        bool log_updates = true
    )
    {
        gaia_ptr<gaia_se_node> node(id, payload_size + sizeof(gaia_se_node), false, log_updates);

        node->id = id;
        node->type = type;
        node->payload_size = payload_size;
        memcpy (node->payload, payload, payload_size);
        return node;
    }

    static gaia_ptr<gaia_se_node> open (
        gaia_id_t id
    )
    {
        auto node = gaia_ptr<gaia_se_node>(id);
        if (!node) {
            throw invalid_node_id(id);
        }
        return node;
    }
};

struct gaia_se_edge
{
    gaia_ptr<gaia_se_node> node_first;
    gaia_ptr<gaia_se_node> node_second;
    gaia_ptr<gaia_se_edge> next_edge_first;
    gaia_ptr<gaia_se_edge> next_edge_second;

    gaia_id_t id;
    gaia_type_t type;
    gaia_id_t first;
    gaia_id_t second;
    size_t payload_size;
    char payload[0];

    static gaia_ptr<gaia_se_edge> create (
        gaia_id_t id,
        gaia_type_t type,
        gaia_id_t first,
        gaia_id_t second,
        size_t payload_size,
        const void* payload,
        bool log_updates = true
    )
    {
        gaia_ptr<gaia_se_node> node_first (first);
        gaia_ptr<gaia_se_node> node_second (second);

        if (!node_first)
        {
            throw invalid_node_id(first);
        }

        if (!node_second)
        {
            throw invalid_node_id(second);
        }

        gaia_ptr<gaia_se_edge> edge(id, payload_size + sizeof(gaia_se_edge), true, log_updates);

        edge->id = id;
        edge->type = type;
        edge->first = first;
        edge->second = second;
        edge->payload_size = payload_size;
        memcpy (edge->payload, payload, payload_size);

        edge->node_first = node_first;
        edge->node_second = node_second;

        edge->next_edge_first = node_first->next_edge_first;
        edge->next_edge_second = node_second->next_edge_second;

        node_first.clone();
        node_second.clone();

        node_first->next_edge_first = edge;
        node_second->next_edge_second = edge;
        return edge;
    }

    static gaia_ptr<gaia_se_edge> open (
        gaia_id_t id
    )
    {
        auto edge = gaia_ptr<gaia_se_edge>(id, true);
        if (!edge) {
            throw invalid_node_id(id);
        }
        return edge;
    }
};

template<>
inline void gaia_ptr<gaia_se_node>::remove (gaia_ptr<gaia_se_node>& node)
{
    if (!node)
    {
        return;
    }
    check_id(node->id);

    if (node->next_edge_first
        || node->next_edge_second)
    {
        throw dependent_edges_exist();
    }
    else
    {
        node.reset();
    }
}

template<>
inline void gaia_ptr<gaia_se_edge>::remove (gaia_ptr<gaia_se_edge>& edge)
{
    if (!edge)
    {
        return;
    }
    check_id(edge->id);

    auto node_first = edge->node_first;
    if (node_first->next_edge_first == edge)
    {
        node_first.clone();
        node_first->next_edge_first = edge->next_edge_first;
    }
    else
    {
        auto current_edge = node_first->next_edge_first;
        for (;;)
        {
            retail_assert(current_edge);
            if (current_edge->next_edge_first == edge)
            {
                current_edge.clone();
                current_edge->next_edge_first = edge->next_edge_first;
                break;
            }
            current_edge = current_edge->next_edge_first;
        }
    }

    auto node_second = edge->node_second;
    if (node_second->next_edge_second == edge)
    {
        node_second.clone();
        node_second->next_edge_second = edge->next_edge_second;
    }
    else
    {
        auto current_edge = node_second->next_edge_second;

        for (;;)
        {
            retail_assert(current_edge);
            if (current_edge->next_edge_second == edge)
            {
                current_edge.clone();
                current_edge->next_edge_second = edge->next_edge_second;
                break;
            }
            current_edge = current_edge->next_edge_second;
        }
    }

    edge.reset();
}

} // db
} // gaia
