/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{
namespace db
{
    using namespace common;

    class gaia_client: private gaia_mem_base
    {
        template<typename T>
        friend class gaia_ptr;
        friend class gaia_hash_map;

    private:
        thread_local static int s_fd_log;

        static gaia_tx_hook s_tx_begin_hook;
        static gaia_tx_hook s_tx_commit_hook;
        static gaia_tx_hook s_tx_rollback_hook;

        // inherited from gaia_mem_base:
        // static int s_fd_offsets;
        // static data *s_data;
        // thread_local static log *s_log;
        // thread_local static int s_session_socket;

        static inline bool is_tx_active()
        {
            return (*s_offsets != nullptr);
        }

        static void tx_cleanup();

        static int get_session_socket();

    protected:
        thread_local static offsets* s_offsets;

    public:
        static inline int64_t allocate_row_id()
        {
            if (*s_offsets == nullptr)
            {
                throw tx_not_open();
            }

            if (s_data->row_id_count >= MAX_RIDS)
            {
                throw oom();
            }

            return 1 + __sync_fetch_and_add (&s_data->row_id_count, 1);
        }

        static void inline allocate_object (int64_t row_id, size_t size)
        {
            if (*s_offsets == nullptr)
            {
                throw tx_not_open();
            }

            if (s_data->objects[0] >= MAX_OBJECTS)
            {
                throw oom();
            }

            (*s_offsets)[row_id] = 1 + __sync_fetch_and_add(
                &s_data->objects[0],
                (size + sizeof(int64_t) -1) / sizeof(int64_t));
        }

        static void create_session();

        static void destroy_session();

        static void begin_transaction();

        static bool commit_transaction();

        static inline void verify_tx_active()
        {
            if (!is_tx_active())
            {
                throw tx_not_open();
            }
        }

        static inline void verify_no_tx()
        {
            if (is_tx_active())
            {
                throw tx_in_progress();
            }
        }

        static inline void verify_session_active()
        {
            if (s_session_socket == -1)
            {
                throw no_session_active();
            }
        }

        static inline void verify_no_session()
        {
            if (s_session_socket != -1)
            {
                throw session_exists();
            }
        }

        static inline void tx_log (int64_t row_id, int64_t old_object, int64_t new_object)
        {
            assert(s_log->count < MAX_LOG_RECS);

            log::log_record* lr = s_log->log_records + s_log->count++;

            lr->row_id = row_id;
            lr->old_object = old_object;
            lr->new_object = new_object;
        }

        // The following setters will never overwrite already-registered hooks.
        static inline bool set_tx_begin_hook(gaia_tx_hook hook)
        {
            return __sync_bool_compare_and_swap(&s_tx_begin_hook, 0, hook);
        }

        static inline bool set_tx_commit_hook(gaia_tx_hook hook)
        {
            return __sync_bool_compare_and_swap(&s_tx_commit_hook, 0, hook);
        }


        static inline bool set_tx_rollback_hook(gaia_tx_hook hook)
        {
            return __sync_bool_compare_and_swap(&s_tx_rollback_hook, 0, hook);
        }

    };

} // db
} // gaia
