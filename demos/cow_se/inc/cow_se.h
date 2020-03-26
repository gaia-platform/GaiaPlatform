/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <iostream>
#include <iomanip>
#include <cassert>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>

#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>

namespace gaia_se
{
    typedef uint64_t gaia_id_t;
    typedef uint64_t gaia_type_t;
    typedef uint64_t gaia_edge_type_t;

    struct gaia_se_node;
    struct gaia_se_edge;
    class gaia_ptr_base;

    class tx_not_open: public std::exception
    {
    public:
        const char* what() const throw ()
        {
            return "begin transaction before performing data access";
        }
    };

    class tx_update_conflict: public std::exception
    {
    public:
        const char* what() const throw ()
        {
            return "transaction aborted due to serialization error";
        }
    };

    class duplicate_id: public std::exception
    {
    public:
        const char* what() const throw ()
        {
            return "object with the same ID already exists";
        }
    };

    class oom: public std::exception
    {
    public:
        const char* what() const throw ()
        {
            return "out of memory";
        }
    };

    class dependent_edges_exist: public std::exception
    {
    public:
        const char* what() const throw ()
        {
            return "cannot remove node - dependent edges exist";
        }
    };

    class invalid_node_id: public std::exception
    {
    public:
        invalid_node_id(int64_t id)
        {
            std::stringstream strs;
            strs << "ID: " << id << " is either invalid or the node is not found";
            whats = strs.str();
        }

        const char* what() const throw ()
        {
             return whats.c_str();
        }
        std::string whats;
    };

    inline void check_id(gaia_id_t id)
    {
        if (id & 0x8000000000000000)
        {
            throw std::invalid_argument("ID must be less than 2^63");
        }
    }

    class gaia_mem_base
    {
    private:
        static void throw_runtime_error(const std::string& info)
        {
            std::stringstream ss;
            ss << info << " - " << (strerror(errno));
            throw std::runtime_error(ss.str());
        }

    public:
        static void init(bool engine = false)
        {
            if (s_data ||
                s_log ||
                s_offsets)
            {
                std::cerr
                    << "Warning: function sequencing error - calling init when there is an open transaction"
                    << std::endl;
            }

            if (s_engine ||
                s_fd_data ||
                s_fd_offsets)
            {
                std::cerr
                    << "Warning: function sequencing error - calling init more than once"
                    << std::endl;
            }

            const auto S_RWALL = 0666;

            umask(0);

            if (umask(0) != 0)
            {
                throw std::runtime_error("unable to set the seurity mask to 0");
            }

            const auto OPEN_FLAGS = engine ? O_CREAT | O_RDWR : O_RDWR;

            if (!s_fd_offsets)
            {
                s_fd_offsets = shm_open(SCH_MEM_OFFSETS, OPEN_FLAGS, S_RWALL);
                if (s_fd_offsets < 0)
                {
                    throw_runtime_error("shm_open failed");
                }
            }

            if (!s_fd_data)
            {
                s_fd_data = shm_open(SCH_MEM_DATA, OPEN_FLAGS, S_RWALL);
                if (s_fd_data < 0)
                {
                    throw std::runtime_error("shm_open failed");
                }
            }

            if (engine)
            {
                if (!s_engine)
                {
                    s_engine = true;

                    if (ftruncate(s_fd_offsets, 0) ||
                        ftruncate(s_fd_data, 0) ||
                        ftruncate(s_fd_offsets, sizeof(offsets)) ||
                        ftruncate(s_fd_data, sizeof(data)))
                    {
                        throw_runtime_error("ftruncate failed");
                    }
                }
            }
        }

        static void tx_begin()
        {
            s_data = (data*)mmap (nullptr, sizeof(data),
                                            PROT_READ|PROT_WRITE, MAP_SHARED, s_fd_data, 0);

            if (MAP_FAILED == s_data)
            {
                throw_runtime_error("mmap failed");
            }

            s_log = (log*)mmap(nullptr, sizeof(log), 
                            PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            if (MAP_FAILED == s_log)
            {
                throw_runtime_error("mmap failed");
            }

            if (flock(s_fd_offsets, LOCK_SH) < 0)
            {
                throw_runtime_error("flock failed");
            }

            s_offsets = (offsets*)mmap (nullptr, sizeof(offsets),
                        PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, s_fd_offsets, 0);

            if (MAP_FAILED == s_offsets)
            {
                throw_runtime_error("mmap failed");
            }
            
            if (flock(s_fd_offsets, LOCK_UN) < 0)
            {
                throw_runtime_error("flock failed");
            }
        }

        static void tx_log (int64_t row_id, int64_t old_object, int64_t new_object)
        {
            assert(s_log->count < MAX_LOG_RECS);

            log::log_record* lr = s_log->log_records + s_log->count++;

            lr->row_id = row_id;
            lr->old_object = old_object;
            lr->new_object = new_object;
        }

        static void tx_commit()
        {
            remap_offsets_shared();

            std::set<int64_t> row_ids;

            for (auto i = 0; i < s_log->count; i++)
            {
                auto lr = s_log->log_records + i;

                if (row_ids.insert(lr->row_id).second)
                {
                    if ((*s_offsets)[lr->row_id] != lr->old_object)
                    {
                        unmap();
                        throw tx_update_conflict();
                    }
                }
            }

            for (auto i = 0; i < s_log->count; i++)
            {
                auto lr = s_log->log_records + i;
                (*s_offsets)[lr->row_id] = lr->new_object;
            }
            unmap();
        }

        static inline void tx_rollback();

    template <typename T>
    friend class gaia_ptr;
    template <typename T>
    friend class gaia_iterator;

    protected:
        const static char* SCH_MEM_OFFSETS;
        const static char* SCH_MEM_DATA;
        const static char* COMMIT_SEM;
        static int s_fd_offsets;
        static int s_fd_data;

        auto static const MAX_RIDS = 32 * 128L * 1024L;
        static const auto HASH_BUCKETS = 12289;
        static const auto HASH_LIST_ELEMENTS = MAX_RIDS;
        static const auto MAX_LOG_RECS = 1000000;
        static const auto MAX_OBJECTS = MAX_RIDS * 8;

        typedef int64_t offsets[MAX_RIDS];

        struct hash_node
        {
            gaia_id_t id;
            int64_t next;
            int64_t row_id;
        };

        struct data
        {
            int64_t commit_lock;
            int64_t row_id_count;
            int64_t hash_node_count;
            hash_node hash_nodes[HASH_BUCKETS + HASH_LIST_ELEMENTS];

            int64_t objects[MAX_RIDS * 8];
        };
    
        struct log
        {
            int64_t count;
            struct log_record {
                int64_t row_id;
                int64_t old_object;
                int64_t new_object;
            } log_records[MAX_LOG_RECS];
        };

        static offsets *s_offsets;
        static data *s_data;
        static log *s_log;
        static bool s_engine;

        static void remap_offsets_shared()
        {
            if (munmap(s_offsets, sizeof(offsets)) < 0)
            {
                throw_runtime_error("munmap failed");
            }

            if (flock(s_fd_offsets, LOCK_EX) < 0)
            {
                throw_runtime_error("flock failed");
            }


            s_offsets = (offsets*)mmap (nullptr, sizeof(offsets),
                                        PROT_READ|PROT_WRITE, MAP_SHARED, s_fd_offsets, 0);

            if (MAP_FAILED == s_offsets)
            {
                throw_runtime_error("mmap failed");
            }
        }

        static void verify_tx_active()
        {
            if (*gaia_mem_base::s_offsets == nullptr)
            {
                throw tx_not_open();
            }
        }

        static void unmap()
        {
            if (flock(s_fd_offsets, LOCK_UN) < 0)
            {
                throw_runtime_error("flock failed");
            }

            if (munmap(s_offsets, sizeof(offsets))  < 0 ||
                munmap(s_data, sizeof(data)) < 0 ||
                munmap(s_log, sizeof(log)) < 0)
            {
                throw_runtime_error("mumap failed");
            }

            s_offsets = nullptr;
            s_data = nullptr;
            s_log = nullptr;
        }

        static int64_t allocate_row_id()
        {
            if (*gaia_mem_base::s_offsets == nullptr)
            {
                throw tx_not_open();
            }

            if (s_data->row_id_count >= MAX_RIDS)
            {
                throw oom();
            }

            return 1 + __sync_fetch_and_add (&s_data->row_id_count, 1);
        }

        static void allocate_object (int64_t row_id, size_t size)
        {
            if (*gaia_mem_base::s_offsets == nullptr)
            {
                throw tx_not_open();
            }

            if (s_data->objects[0] >= MAX_OBJECTS)
            {
                throw oom();
            }

            (*s_offsets)[row_id] = 1 + __sync_fetch_and_add (&s_data->objects[0],
                                        (size + sizeof(int64_t) -1) / sizeof(int64_t));
        }

        static void* offset_to_ptr(int64_t offset)
        {
            return offset && (*gaia_mem_base::s_offsets)[offset]
                ? (gaia_mem_base::s_data->objects
                             + (*gaia_mem_base::s_offsets)[offset])
                : nullptr;
        }

        static int64_t ptr_to_offset(void* ptr)
        {
            auto iptr = (int64_t*)ptr;

            if (iptr <= (*gaia_mem_base::s_offsets) ||
                iptr >= (*gaia_mem_base::s_offsets) + MAX_RIDS)
            {
                return 0;
            }

            return iptr - (*gaia_mem_base::s_offsets);
        }
    };

    const char* gaia_mem_base::SCH_MEM_OFFSETS = "gaia_mem_offsets";
    const char* gaia_mem_base::SCH_MEM_DATA = "gaia_mem_data";
    int gaia_mem_base::s_fd_offsets = 0;
    int gaia_mem_base::s_fd_data = 0;
    bool gaia_mem_base::s_engine = false;

    class gaia_hash_map: public gaia_mem_base
    {
    public:
        static hash_node* insert(const gaia_id_t id)
        {
            if (*gaia_mem_base::s_offsets == nullptr)
            {
                throw tx_not_open();
            }

            hash_node* node = s_data->hash_nodes + (id % HASH_BUCKETS);
            if (node->id == 0 && __sync_bool_compare_and_swap(&node->id, 0, id))
            {
                return node;
            }

            int64_t new_node_idx = 0;

            for (;;)
            {
                __sync_synchronize();

                if (node->id == id)
                {
                    if (node->row_id &&
                        (*gaia_mem_base::s_offsets)[node->row_id])
                    {
                        throw duplicate_id();
                    }
                    else
                    {
                        return node;
                    }
                }

                if (node->next)
                {
                    node = s_data->hash_nodes + node->next;
                    continue;
                }

                if (!new_node_idx)
                {
                    assert(s_data->hash_node_count + HASH_BUCKETS < HASH_LIST_ELEMENTS);
                    new_node_idx = HASH_BUCKETS +
                                     __sync_fetch_and_add (&s_data->hash_node_count, 1);
                    (s_data->hash_nodes + new_node_idx)->id = id;
                }

                if (__sync_bool_compare_and_swap(&node->next, 0, new_node_idx))
                {
                    return s_data->hash_nodes + new_node_idx;
                }
            }
        }

        static int64_t find(const gaia_id_t id)
        {
            if (*gaia_mem_base::s_offsets == nullptr)
            {
                throw tx_not_open();
            }

            auto node = s_data->hash_nodes + (id % HASH_BUCKETS);

            while (node) 
            {
                if (node->id == id)
                {
                    if (node->row_id && (*gaia_mem_base::s_offsets)[node->row_id])
                    {
                        return node->row_id;
                    }
                    else
                    {
                        return 0;
                    }
                }

                node = node->next
                    ? s_data->hash_nodes + node->next
                    : 0;
            }

            return 0;
        }

        static void remove(const gaia_id_t id)
        {
            hash_node* node = s_data->hash_nodes + (id % HASH_BUCKETS);

            while (node->id)
            {
                if (node->id == id)
                {
                    if (node->row_id)
                    {
                        node->row_id = 0;
                    }
                    return;
                }
                if (!node->next)
                {
                    return;
                }
                node = s_data->hash_nodes + node->next;
            }
        }

    };

    inline void gaia_mem_base::tx_rollback()
    {
        for (auto i = 0; i < s_log->count; i++)
        {
            auto lr = s_log->log_records + i;
            gaia_hash_map::remove(lr->row_id);
        }
        unmap();
    }

    template <typename T>
    class gaia_ptr
    {
        // These two structs need to access the gaia_ptr protected constructors.
        friend struct gaia_se_node;
        friend struct gaia_se_edge;

    public:
        gaia_ptr (const std::nullptr_t null = nullptr)
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

        bool operator == (const std::nullptr_t null) const
        {
            return to_ptr() == nullptr;
        }

        bool operator != (const std::nullptr_t null) const
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

            gaia_mem_base::tx_log (row_id, old_offset, to_offset());

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

            gaia_mem_base::tx_log (row_id, old_offset, to_offset());

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
            return row_id;
        }

        static void remove (gaia_ptr<T>&);

    protected:
        gaia_ptr (const gaia_id_t id, bool is_edge = false)
        {
            gaia_id_t id_copy = preprocess_id(id, is_edge);

            row_id = gaia_hash_map::find(id_copy);
        }

        gaia_ptr (const gaia_id_t id, const size_t size, bool is_edge = false)
            :row_id(0)
        {
            gaia_id_t id_copy = preprocess_id(id, is_edge);

            gaia_hash_map::hash_node* hash_node = gaia_hash_map::insert (id_copy);
            hash_node->row_id = row_id = gaia_mem_base::allocate_row_id();
            gaia_mem_base::allocate_object(row_id, size);

            gaia_mem_base::tx_log (row_id, 0, to_offset());
        }

        gaia_id_t preprocess_id(const gaia_id_t& id, bool is_edge = false)
        {
            check_id(id);

            gaia_id_t id_copy = id;
            if (is_edge)
            {
                id_copy = id_copy | 0x8000000000000000;
            }
            return id_copy;
        }

        void allocate (const size_t size)
        {
            gaia_mem_base::allocate_object(row_id, size);
        }

        T* to_ptr() const
        {
            gaia_mem_base::verify_tx_active();

            return row_id && (*gaia_mem_base::s_offsets)[row_id]
                ? (T*)(gaia_mem_base::s_data->objects
                             + (*gaia_mem_base::s_offsets)[row_id])
                : nullptr;
        }

        int64_t to_offset() const
        {
            gaia_mem_base::verify_tx_active();

            return row_id
                ? (*gaia_mem_base::s_offsets)[row_id]
                : 0;
        }

        bool is (gaia_type_t type) const
        {
            return to_ptr() && to_ptr()->type == type;
        }

        void find_next(gaia_type_t type)
        {
            // search for rows of this type within the range of used slots
            while (++row_id && row_id < gaia_mem_base::s_data->row_id_count+1)
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
            gaia_mem_base::tx_log (row_id, to_offset(), 0);
            (*gaia_mem_base::s_offsets)[row_id] = 0;
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

        static gaia_ptr<gaia_se_node> create (
            gaia_id_t id,
            gaia_type_t type,
            size_t payload_size,
            const void* payload
        )
        {
            gaia_ptr<gaia_se_node> node(id, payload_size + sizeof(gaia_se_node));

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
            return gaia_ptr<gaia_se_node>(id);
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
            const void* payload
        )
        {
            gaia_ptr<gaia_se_node> node_first = gaia_se_node::open(first);
            gaia_ptr<gaia_se_node> node_second = gaia_se_node::open(second);

            if (!node_first)
            {
                throw invalid_node_id(first);
            }

            if (!node_second)
            {
                throw invalid_node_id(second);
            }

            gaia_ptr<gaia_se_edge> edge(id, payload_size + sizeof(gaia_se_edge), true);

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
            return gaia_ptr<gaia_se_edge>(id, true);
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

        if (node->next_edge_first ||
            node->next_edge_second)
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
                assert(current_edge);
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
                assert(current_edge);
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

    inline void begin_transaction()
    {
        gaia_mem_base::tx_begin();
    }

    inline void commit_transaction()
    {
        gaia_mem_base::tx_commit();
    }

    inline void rollback_transaction()
    {
        gaia_mem_base::tx_rollback();
    }

    gaia_mem_base::offsets *gaia_mem_base::s_offsets = nullptr;
    gaia_mem_base::data *gaia_mem_base::s_data = nullptr;
    gaia_mem_base::log *gaia_mem_base::s_log = nullptr;
}
