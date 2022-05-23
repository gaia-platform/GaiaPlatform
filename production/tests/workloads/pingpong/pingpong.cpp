////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "json.hpp"

#include "gaia_pingpong.h"
#include "simulation.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::direct_access;
using namespace gaia::simulation::exceptions;
using namespace gaia::rules;

using namespace gaia::pingpong;

using json_t = nlohmann::json;

class pingpong_simulation_t : public simulation_t
{
private:
    static constexpr int c_default_json_indentation = 4;

public:
    gaia_id_t insert_ping_table(const char* name)
    {
        ping_table_writer ping_table_w;
        ping_table_w.name = name;
        ping_table_w.step_timestamp = 0;
        return ping_table_w.insert_row();
    }

    gaia_id_t insert_pong_table(const char* name)
    {
        pong_table_writer pong_table_w;
        pong_table_w.name = name;
        pong_table_w.rule_timestamp = 0;
        return pong_table_w.insert_row();
    }

    gaia_id_t insert_marco_table(const char* name)
    {
        marco_table_writer marco_table_w;
        marco_table_w.name = name;
        marco_table_w.marco_count = 0;
        marco_table_w.marco_limit = 0;
        return marco_table_w.insert_row();
    }

    gaia_id_t insert_polo_table(const char* name)
    {
        polo_table_writer polo_table_w;
        polo_table_w.name = name;
        polo_table_w.polo_count = 0;
        return polo_table_w.insert_row();
    }

    void restore_ping_table(ping_table_t& ping_table)
    {
        ping_table_writer ping_table_w = ping_table.writer();
        ping_table_w.step_timestamp = 0;
        ping_table_w.update_row();
    }

    void restore_pong_table(pong_table_t& pong_table)
    {
        pong_table_writer pong_table_w = pong_table.writer();
        pong_table_w.rule_timestamp = 0;
        pong_table_w.update_row();
    }

    void restore_marco_table(int required_iterations)
    {
        for (auto& marco_table : marco_table_t::list())
        {
            marco_table_writer marco_table_w = marco_table.writer();
            marco_table_w.marco_count = 0;
            marco_table_w.marco_limit = required_iterations;
            marco_table_w.update_row();
        }
    }

    void restore_polo_table(polo_table_t& polo_table)
    {
        polo_table_writer polo_table_w = polo_table.writer();
        polo_table_w.polo_count = 0;
        polo_table_w.update_row();
    }

    void restore_default_values()
    {
        for (auto& ping_table : ping_table_t::list())
        {
            restore_ping_table(ping_table);
        }
        for (auto& pong_table : pong_table_t::list())
        {
            restore_pong_table(pong_table);
        }
        restore_marco_table(0);
        for (auto& polo_table : polo_table_t::list())
        {
            restore_polo_table(polo_table);
        }
    }

    void init_storage() override
    {
        printf("here\n");
        auto_transaction_t tx(auto_transaction_t::no_auto_restart);
        printf("here\n");

        // If we already have inserted a ping pong table then our storage has already been
        // initialized.  Re-initialize the database to default values.
        if (ping_table_t::list().size())
        {
            restore_default_values();
            tx.commit();
            return;
        }

        // Single ping pong table
        const char c_main[] = "main";
        auto ping_table = ping_table_t::get(insert_ping_table(c_main));
        auto pong_table = pong_table_t::get(insert_pong_table(c_main));
        auto marco_table = marco_table_t::get(insert_marco_table(c_main));
        auto polo_table = polo_table_t::get(insert_polo_table(c_main));
        tx.commit();
    }

    json_t to_json(const ping_table_t& ping_table)
    {
        json_t json;
        json["step_timestamp"] = ping_table.step_timestamp();
        json["name"] = ping_table.name();
        return json;
    }

    json_t to_json(const pong_table_t& pong_table)
    {
        json_t json;
        json["rule_timestamp"] = pong_table.rule_timestamp();
        json["name"] = pong_table.name();
        return json;
    }

    json_t to_json(const marco_table_t& marco_table)
    {
        json_t json;
        json["name"] = marco_table.name();
        json["marco_count"] = marco_table.marco_count();
        json["marco_limit"] = marco_table.marco_limit();
        return json;
    }

    json_t to_json(const polo_table_t& polo_table)
    {
        json_t json;
        json["name"] = polo_table.name();
        json["polo_count"] = polo_table.polo_count();
        return json;
    }

    void dump_db_json(FILE* object_log_file) override
    {
        begin_transaction();
        json_t json_document;

        for (const ping_table_t& ping_table : ping_table_t::list())
        {
            json_document["ping_table"].push_back(to_json(ping_table));
        }
        for (const pong_table_t& pong_table : pong_table_t::list())
        {
            json_document["pong_table"].push_back(to_json(pong_table));
        }
        for (const marco_table_t& marco_table : marco_table_t::list())
        {
            json_document["marco_table"].push_back(to_json(marco_table));
        }
        for (const polo_table_t& polo_table : polo_table_t::list())
        {
            json_document["polo_table"].push_back(to_json(polo_table));
        }

        fprintf(object_log_file, "%s", json_document.dump(c_default_json_indentation).c_str());
        commit_transaction();
    }

    void setup_test_data(int required_iterations) override
    {
        restore_marco_table(required_iterations);
    }

    bool has_test_completed() override
    {
        int times_to_charging = -1;
        int target_times_to_charge = -2;
        begin_transaction();
        for (const marco_table_t& i : marco_table_t::list())
        {
            times_to_charging = i.marco_count();
            target_times_to_charge = i.marco_limit();
        }
        commit_transaction();

        return times_to_charging == target_times_to_charge;
    }

    my_duration_in_microseconds_t perform_single_step() override
    {
        my_duration_in_microseconds_t update_duration;
        for (auto ping_table : ping_table_t::list())
        {
            ping_table_writer ping_table_w = ping_table.writer();
            ping_table_w.step_timestamp = g_timestamp;

            my_time_point_t update_start_mark = my_clock_t::now();
            ping_table_w.update_row();
            my_time_point_t update_end_mark = my_clock_t::now();
            update_duration = update_end_mark - update_start_mark;
        }
        return update_duration;
    }

    bool has_step_completed(int timestamp, int initial_state_tracker) override
    {
        if (get_use_transaction_wait())
        {
            int found_timestamp = -1;
            begin_transaction();
            for (const gaia::pingpong::pong_table_t& i : pong_table_t::list())
            {
                found_timestamp = i.rule_timestamp();
            }
            commit_transaction();

            return found_timestamp != timestamp;
        }
        int g_t = g_rule_1_tracker;
        return g_t != initial_state_tracker;
    }

    long get_processing_timeout_in_microseconds() override
    {
        return simulation_t::get_processing_timeout_in_microseconds();
    }

    string get_configuration_file_name() override
    {
        return "pingpong.conf";
    }
};

int main(int argc, const char** argv)
{
    pingpong_simulation_t this_simulation;
    return this_simulation.main(argc, argv);
}
