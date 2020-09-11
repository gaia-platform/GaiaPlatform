/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <queue>
#include <functional>
#include <algorithm>

#include "flatbuffers/idl.h"
#include "tabulate/table.hpp"

#include "command.hpp"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "gaia_event_log.h"

using namespace gaia::catalog;

typedef std::vector<variant<std::string, const char*, tabulate::Table>> row_t;

constexpr const char c_match_all_pattern[] = ".*";
constexpr const char c_describe_command = 'd';
constexpr const char c_generate_command = 'g';
constexpr const char c_list_command = 'l';

template <typename T_obj, typename T_predicator, typename T_fetcher>
void list_catalog_obj(const row_t& header, T_predicator is_match, T_fetcher get_row) {
    tabulate::Table output_table;
    output_table.add_row(header);
    {
        auto_transaction_t tx;
        for (auto obj : T_obj::list()) {
            if (is_match(obj)) {
                output_table.add_row(get_row(obj));
            }
        }
    }

    output_table.print(cout);
    std::cout << endl
              << flush;
}

void list_tables(const regex& re) {
    list_catalog_obj<gaia_table_t>(
        {"Database", "Name", "ID"},
        [&re](gaia_table_t& t) -> bool {
            return regex_match(t.name(), re);
        },
        [](gaia_table_t& t)
            -> row_t {
            return {t.gaia_database().name(), t.name(), to_string(t.gaia_id())};
        });
}

void list_databases(const regex& re) {
    list_catalog_obj<gaia_database_t>(
        {"Name", "ID"},
        [&re](gaia_database_t& d) -> bool {
            return regex_match(d.name(), re);
        },
        [](gaia_database_t& d) -> row_t {
            return {d.name(), to_string(d.gaia_id())};
        });
}

void list_fields(const regex& re) {
    list_catalog_obj<gaia_field_t>(
        {"Table", "Name", "Type", "Repeated Count", "Position", "ID"},
        [&re](gaia_field_t& f) -> bool {
            if (f.type() == static_cast<uint8_t>(data_type_t::e_references)) {
                return false;
            }
            return regex_match(f.name(), re);
        },
        [](gaia_field_t& f) -> row_t {
            return {f.gaia_table().name(), f.name(), to_string(f.type()),
                to_string(f.repeated_count()), to_string(f.position()), to_string(f.gaia_id())};
        });
}

void list_references(const regex& re) {
    list_catalog_obj<gaia_field_t>(
        {"Table", "Name", "Parent", "Position", "ID"},
        [&re](gaia_field_t& f) -> bool {
            if (f.type() != static_cast<uint8_t>(data_type_t::e_references)) {
                return false;
            }
            return regex_match(f.name(), re);
        },
        [](gaia_field_t& f) -> row_t {
            return {f.gaia_table().name(), f.name(),
                f.ref_gaia_table().name(), to_string(f.position()), to_string(f.gaia_id())};
        });
}

void show_event_log() {
    list_catalog_obj<gaia::event_log::event_log_t>(
        {"Event Type", "Type", "Record ID", "Column", "Timestamp", "Rules Invoked"},
        [](gaia::event_log::event_log_t&) -> bool { return true; },
        [](gaia::event_log::event_log_t& e) -> row_t {
            return {to_string(e.event_type()),
                gaia_table_t::get(e.type_id()).name(),
                to_string(e.record_id()),
                e.column_id() == INVALID_GAIA_ID ? "" : gaia_field_t::get(e.column_id()).name(),
                to_string(e.timestamp()),
                to_string(e.rules_invoked())};
        });
}

void describe_database(const string& name) {
    tabulate::Table output_table;
    output_table.add_row({"Name"});
    bool db_exists = false;
    {
        auto_transaction_t tx;
        for (auto db : gaia_database_t::list()) {
            if (string(db.name()) == name) {
                for (auto table : db.gaia_table_list()) {
                    cout << name << endl;
                    output_table.add_row({table.name()});
                }
                db_exists = true;
                break;
            }
        }
    }
    if (!db_exists) {
        throw db_not_exists(name);
    }

    cout << "Information of database \"" << name << "\"" << endl;
    cout << endl;
    cout << "List of tables:" << endl;
    output_table.print(cout);
    cout << endl;
    cout << flush;
}

void describe_table(const string& name) {
    tabulate::Table output_fields, output_references;
    output_fields.add_row({"Name", "Type", "Repeated Count", "Position"});
    output_references.add_row({"Name", "Parent", "Position"});
    bool table_exists = false;
    {
        auto_transaction_t tx;
        for (auto table : gaia_table_t::list()) {
            string table_name(table.name());
            string db_name(table.gaia_database().name());
            if (name == table_name || name == (db_name + "." + table_name)) {
                for (auto field : table.gaia_field_list()) {
                    if (field.type() != static_cast<uint8_t>(data_type_t::e_references)) {
                        output_fields.add_row({field.name(), to_string(field.type()),
                            to_string(field.repeated_count()), to_string(field.position())});
                    } else {
                        output_references.add_row({field.name(),
                            field.ref_gaia_table().name(), to_string(field.position())});
                    }
                }
                table_exists = true;
                break;
            }
        }
    }
    if (!table_exists) {
        throw table_not_exists(name);
    }

    cout << "Information of table \"" << name << "\"" << endl;
    cout << endl;
    cout << "List of fields:" << endl;
    output_fields.print(cout);
    cout << endl;
    cout << endl;
    cout << "List of references:" << endl;
    output_references.print(cout);
    cout << endl;
    cout << flush;
}

void generate_table_fbs(const string& name) {
    gaia_id_t table_id = INVALID_GAIA_ID;
    {
        auto_transaction_t tx;
        for (auto table : gaia_table_t::list()) {
            string table_name(table.name());
            string db_name(table.gaia_database().name());
            if (name == table_name || name == (db_name + "." + table_name)) {
                table_id = table.gaia_id();
                break;
            }
        }
    }
    if (table_id == INVALID_GAIA_ID) {
        throw table_not_exists(name);
    }
    cout << generate_fbs(table_id) << endl;
    cout << flush;
}


regex parse_pattern(const string& cmd, size_t pos) {
    if (cmd.length() <= pos) {
        return regex(c_match_all_pattern);
    }
    if (cmd[pos] != ' ') {
        throw invalid_command(cmd);
    }
    std::size_t found = cmd.find_first_not_of(' ', pos);
    if (found != string::npos) {
        try {
            return regex(cmd.substr(found));
        } catch (std::regex_error& e) {
            throw invalid_command(cmd, e);
        }
    }
    return regex(c_match_all_pattern);
}

string parse_name(const string& cmd, size_t pos) {
    if (cmd.length() <= pos) {
        throw invalid_command(cmd);
    }
    if (cmd[pos] != ' ') {
        throw invalid_command(cmd);
    }
    std::size_t found = cmd.find_first_not_of(' ', pos);
    if (found != string::npos) {
        return cmd.substr(found);
    }
    throw invalid_command(cmd);
}

void handle_describe_command(const string& cmd) {
    retail_assert(cmd.length() > 2);
    retail_assert(cmd[0] == c_command_prefix);
    retail_assert(cmd[1] == c_describe_command);

    switch (cmd[2]) {
    case ' ':
        describe_table(parse_name(cmd, 2));
        break;
    case 'd':
        describe_database(parse_name(cmd, 3));
        break;
    case 't':
        describe_table(parse_name(cmd, 3));
        break;
    default:
        throw invalid_command(cmd);
    }
}

void handle_list_command(const string& cmd) {
    retail_assert(cmd.length() > 1);
    retail_assert(cmd[0] == c_command_prefix);
    retail_assert(cmd[1] == c_list_command);

    if (cmd.length() == 2) {
        return list_tables(regex(c_match_all_pattern));
    }

    switch (cmd[2]) {
    case ' ':
        list_tables(parse_pattern(cmd, 2));
        break;
    case 'd':
        list_databases(parse_pattern(cmd, 3));
        break;
    case 'e':
        show_event_log();
        break;
    case 'f':
        list_fields(parse_pattern(cmd, 3));
        break;
    case 'r':
        list_references(parse_pattern(cmd, 3));
        break;
    case 't':
        list_tables(parse_pattern(cmd, 3));
        break;
    default:
        throw invalid_command(cmd);
    }
}

void handle_generate_command(const string& cmd) {
    retail_assert(cmd.length() > 2);
    retail_assert(cmd[0] == c_command_prefix);
    retail_assert(cmd[1] == c_generate_command);

    switch (cmd[2]) {
    case ' ':
        generate_fbs(parse_name(cmd, 2));
        break;
    case 'd':
        generate_fbs(parse_name(cmd, 3));
        break;
    case 't':
        generate_table_fbs(parse_name(cmd, 3));
        break;
    default:
        throw invalid_command(cmd);
    }
}

string command_usage() {
    std::stringstream ss;
    ss << "Usage\n"
          "  \\dd   NAME      Describe the database of the given NAME.\n"
          "  \\d[t] NAME      Describe the table of the given NAME.\n"
          "  \\ld   [PATTERN] List databases optionally filtering by the PATTERN.\n"
          "  \\le             List event log entries.\n"
          "  \\lf   [PATTERN] List data fields optionally filtering by the PATTERN.\n"
          "  \\lr   [PATTERN] List references optionally filtering by the PATTERN.\n"
          "  \\l[t] [PATTERN] List tables optionally filtering by the PATTERN.\n"
          "  \\g[d] NAME      Generate fbs for a given database.\n"
          "  \\gt   NAME      Generate fbs for a given table.\n"
          "  \\h              Print help information.\n";
    return ss.str();
}

void handle_slash_command(const string& cmd) {
    retail_assert(!cmd.empty(), "Slash command should not be empty.");
    retail_assert(cmd[0] == '\\', "Slash command should start with a forward slash.");
    if (cmd.length() < 2) {
        throw invalid_command(cmd);
    }
    switch (cmd[1]) {
    case c_list_command:
        handle_list_command(cmd);
        break;
    case c_describe_command:
        handle_describe_command(cmd);
        break;
    case c_generate_command:
        handle_generate_command(cmd);
        break;
    case 'h':
        cout << command_usage() << endl
             << flush;
        break;
    default:
        throw gaia_exception("Invalid command " + string(cmd));
    }
}
