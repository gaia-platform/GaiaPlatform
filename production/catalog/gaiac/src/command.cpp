/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "command.hpp"

#include <functional>
#include <queue>

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/retail_assert.hpp"
#include "tabulate/table.hpp"

#include "fbs_generator.hpp"

using namespace std;

using namespace gaia::direct_access;
using namespace gaia::common;
using namespace gaia::catalog;

typedef std::vector<variant<std::string, const char*, tabulate::Table>> row_t;

namespace
{ // Use unnamed namespace to restrict external linkage.

// The regular expression pattern to match everything.
constexpr char c_match_all_pattern[] = ".*";

// Character literals for meta command parsing.
constexpr char c_describe_command = 'd';
#ifdef DEBUG
// Hide FlatBuffers related commands in release build.
constexpr char c_generate_command = 'g';
#endif
constexpr char c_list_command = 'l';
constexpr char c_help_command = 'h';
constexpr char c_exit_command = 'q';
constexpr char c_command_separator = ' ';

constexpr char c_db_subcommand = 'd';
constexpr char c_table_subcommand = 't';
constexpr char c_field_subcommand = 'f';
constexpr char c_relationship_subcommand = 'r';

// Misc size and index constants that are used to parse command strings.
constexpr size_t c_cmd_minimum_length = 2;
constexpr size_t c_cmd_prefix_index = 0;
constexpr size_t c_command_index = 1;
constexpr size_t c_subcommand_index = 2;

// Title names for catalog table columns.
constexpr char c_name_title[] = "Name";
constexpr char c_id_title[] = "ID";
constexpr char c_database_title[] = "Database";
constexpr char c_table_title[] = "Table";
constexpr char c_type_title[] = "Type";
constexpr char c_position_title[] = "Position";
constexpr char c_repeated_count_title[] = "Repeated Count";
constexpr char c_parent_title[] = "Parent";
constexpr char c_child_title[] = "Child";

template <typename T_obj>
void list_catalog_obj(const row_t& header, function<bool(T_obj&)> is_match, function<row_t(T_obj&)> get_row)
{
    tabulate::Table output_table;
    output_table.add_row(header);

    {
        auto_transaction_t tx;
        for (auto obj : T_obj::list())
        {
            if (is_match(obj))
            {
                output_table.add_row(get_row(obj));
            }
        }
    }

    output_table.print(cout);
    std::cout << endl;
}

void list_tables(const regex& re)
{
    list_catalog_obj<gaia_table_t>(
        {c_database_title, c_name_title, c_id_title, c_type_title},
        [&re](gaia_table_t& t) -> bool { return regex_match(t.name(), re); },
        [](gaia_table_t& t) -> row_t {
            return {t.gaia_database().name(), t.name(), to_string(t.gaia_id()), to_string(t.type())};
        });
}

void list_databases(const regex& re)
{
    list_catalog_obj<gaia_database_t>(
        {c_name_title, c_id_title}, [&re](gaia_database_t& d) -> bool { return regex_match(d.name(), re); },
        [](gaia_database_t& d) -> row_t {
            return {d.name(), to_string(d.gaia_id())};
        });
}

void list_fields(const regex& re)
{
    list_catalog_obj<gaia_field_t>(
        {c_table_title, c_name_title, c_type_title, c_repeated_count_title, c_position_title, c_id_title},
        [&re](gaia_field_t& f) -> bool {
            return regex_match(f.name(), re);
        },
        [](gaia_field_t& f) -> row_t {
            return {
                f.gaia_table().name(),
                f.name(),
                get_data_type_name(static_cast<data_type_t>(f.type())),
                to_string(f.repeated_count()),
                to_string(f.position()),
                to_string(f.gaia_id())};
        });
}

void list_relationships(const regex& re)
{
    list_catalog_obj<gaia_relationship_t>(
        {c_name_title, c_parent_title, c_child_title, c_id_title},
        [&re](gaia_relationship_t& r) -> bool {
            return regex_match(r.name(), re);
        },
        [](gaia_relationship_t& r) -> row_t {
            return {
                r.name(),
                r.parent_gaia_table().name(),
                r.child_gaia_table().name(),
                to_string(r.gaia_id())};
        });
}

void describe_database(const string& name)
{
    tabulate::Table output_table;
    output_table.add_row({c_name_title});
    gaia_id_t db_id = find_db_id(name);
    if (db_id == c_invalid_gaia_id)
    {
        throw db_not_exists(name);
    }
    {
        auto_transaction_t txn;
        for (auto const& table : gaia_database_t::get(db_id).gaia_table_list())
        {
            output_table.add_row({table.name()});
        }
    }
    cout << "Database '" << (name.empty() ? c_empty_db_name : name) << "':" << endl;
    cout << endl;
    cout << "Tables:" << endl;
    output_table.print(cout);
    cout << endl;
    cout << flush;
}

void describe_table(const string& name)
{
    tabulate::Table output_fields, output_child_references, output_parent_references;
    output_fields.add_row({c_name_title, c_type_title, c_repeated_count_title, c_position_title, c_id_title});
    output_child_references.add_row({c_name_title, c_parent_title, c_id_title});
    output_parent_references.add_row({c_name_title, c_child_title, c_id_title});
    gaia_id_t table_id = c_invalid_gaia_id;
    {
        auto_transaction_t tx;
        for (auto& table : gaia_table_t::list())
        {
            string table_name{table.name()};
            string qualified_name{table.gaia_database().name()};
            qualified_name += c_db_table_name_connector;
            qualified_name += table_name;
            if (name == table_name || name == qualified_name)
            {
                table_id = table.gaia_id();
                break;
            }
        }
        if (table_id == c_invalid_gaia_id)
        {
            throw table_not_exists(name);
        }
        for (auto& field : gaia_table_t::get(table_id).gaia_field_list())
        {
            output_fields.add_row(
                {field.name(),
                 get_data_type_name(static_cast<data_type_t>(field.type())),
                 to_string(field.repeated_count()),
                 to_string(field.position()),
                 to_string(field.gaia_id())});
        }
        for (auto& relationship : gaia_table_t::get(table_id).child_gaia_relationship_list())
        {
            output_child_references.add_row(
                {relationship.name(),
                 relationship.parent_gaia_table().name(),
                 to_string(relationship.gaia_id())});
        }
        for (auto& relationship : gaia_table_t::get(table_id).parent_gaia_relationship_list())
        {
            output_parent_references.add_row(
                {relationship.name(),
                 relationship.child_gaia_table().name(),
                 to_string(relationship.gaia_id())});
        }
    }
    cout << "Table '" << name << "':" << endl;
    cout << endl;
    cout << "Fields:" << endl;
    output_fields.print(cout);
    cout << endl;
    cout << endl;
    cout << "Child references:" << endl;
    output_child_references.print(cout);
    cout << endl;
    cout << endl;
    cout << "Parent references:" << endl;
    output_parent_references.print(cout);
    cout << endl;
#ifdef DEBUG
    // Hide FlatBuffers related content in release build.
    {
        cout << endl;
        cout << "Binary FlatBuffers Schema (in hex):" << endl;
        auto_transaction_t tx;
        cout << gaia_table_t::get(table_id).binary_schema() << endl;
    }
#endif
}

#ifdef DEBUG
// Hide FlatBuffers related commands in release build.
void generate_table_fbs(const string& name)
{
    gaia_id_t table_id = c_invalid_gaia_id;
    {
        auto_transaction_t tx;
        for (auto table : gaia_table_t::list())
        {
            string table_name{table.name()};
            string qualified_name{table.gaia_database().name()};
            qualified_name += c_db_table_name_connector;
            qualified_name += table_name;
            if (name == table_name || name == qualified_name)
            {
                table_id = table.gaia_id();
                break;
            }
        }
    }
    if (table_id == c_invalid_gaia_id)
    {
        throw table_not_exists(name);
    }
    cout << generate_fbs(table_id) << endl;
    cout << flush;
}
#endif

regex parse_pattern(const string& cmd, size_t pos)
{
    if (cmd.length() <= pos)
    {
        return regex(c_match_all_pattern);
    }
    if (cmd[pos] != c_command_separator)
    {
        throw invalid_command(cmd);
    }
    size_t found_pos = cmd.find_first_not_of(c_command_separator, pos);
    if (found_pos != string::npos)
    {
        try
        {
            return regex(cmd.substr(found_pos));
        }
        catch (std::regex_error& e)
        {
            throw invalid_command(cmd, e);
        }
    }
    return regex(c_match_all_pattern);
}

string parse_name(const string& cmd, size_t pos, bool throw_on_empty = true)
{
    if (!throw_on_empty && cmd.find_first_not_of(c_command_separator, pos) == string::npos)
    {
        return "";
    }
    if (cmd.length() <= pos)
    {
        throw invalid_command(cmd);
    }
    if (cmd[pos] != c_command_separator)
    {
        throw invalid_command(cmd);
    }
    std::size_t found = cmd.find_first_not_of(c_command_separator, pos);
    if (found != string::npos)
    {
        return cmd.substr(found);
    }
    throw invalid_command(cmd);
}

void handle_describe_command(const string& cmd)
{
    retail_assert(cmd.length() > c_cmd_minimum_length, "Unexpected command length!");
    retail_assert(cmd[c_cmd_prefix_index] == c_command_prefix, "Unexpected command prefix!");
    retail_assert(cmd[c_command_index] == c_describe_command, "Unexpected command!");

    switch (cmd[c_subcommand_index])
    {
    case c_command_separator:
        describe_table(parse_name(cmd, c_subcommand_index));
        break;
    case c_db_subcommand:
        // Describe database can take an empty db name.
        describe_database(parse_name(cmd, c_subcommand_index + 1, false));
        break;
    case c_table_subcommand:
        describe_table(parse_name(cmd, c_subcommand_index + 1));
        break;
    default:
        throw invalid_command(cmd);
    }
}

void handle_list_command(const string& cmd)
{
    retail_assert(cmd.length() > 1, "Unexpected command length!");
    retail_assert(cmd[c_cmd_prefix_index] == c_command_prefix, "Unexpected command prefix!");
    retail_assert(cmd[c_command_index] == c_list_command, "Unexpected command!");

    if (cmd.length() == c_cmd_minimum_length)
    {
        return list_tables(regex(c_match_all_pattern));
    }

    switch (cmd[c_subcommand_index])
    {
    case c_command_separator:
        list_tables(parse_pattern(cmd, c_subcommand_index));
        break;
    case c_db_subcommand:
        list_databases(parse_pattern(cmd, c_subcommand_index + 1));
        break;
    case c_field_subcommand:
        list_fields(parse_pattern(cmd, c_subcommand_index + 1));
        break;
    case c_relationship_subcommand:
        list_relationships(parse_pattern(cmd, c_subcommand_index + 1));
        break;
    case c_table_subcommand:
        list_tables(parse_pattern(cmd, c_subcommand_index + 1));
        break;
    default:
        throw invalid_command(cmd);
    }
}

#ifdef DEBUG
// Hide FlatBuffers related commands in release build.
void handle_generate_command(const string& cmd)
{
    retail_assert(cmd.length() > c_cmd_minimum_length, "Unexpected command length!");
    retail_assert(cmd[c_cmd_prefix_index] == c_command_prefix, "Unexpected command prefix!");
    retail_assert(cmd[c_command_index] == c_generate_command, "Unexpected command!");

    switch (cmd[c_subcommand_index])
    {
    case c_command_separator:
        // Generate fbs can take an empty db name.
        cout << generate_fbs(parse_name(cmd, c_subcommand_index, false)) << endl;
        break;
    case c_db_subcommand:
        // Generate fbs can take an empty db name.
        cout << generate_fbs(parse_name(cmd, c_subcommand_index + 1, false)) << endl;
        break;
    case c_table_subcommand:
        generate_table_fbs(parse_name(cmd, c_subcommand_index + 1));
        break;
    default:
        throw invalid_command(cmd);
    }
}
#endif

inline string optionalize(const string& s)
{
    return "[" + s + "]";
}

inline string optionalize(const char c)
{
    return optionalize(string(1, c));
}

string command_usage()
{
    std::stringstream ss;
    ss << "Usage\n";

    const string name = "NAME";
    const string pattern = "PATTERN";

    tabulate::Table output_table;
    output_table.add_row(
        {string() + c_command_prefix + c_describe_command + c_db_subcommand,
         optionalize(name),
         "Describe the database of the given " + name + "."});
    output_table.add_row(
        {"", "", "Without specifying a name, it will show tables in the " + c_empty_db_name + " database."});
    output_table.add_row(
        {string() + c_command_prefix + c_describe_command + optionalize(c_table_subcommand),
         name, "Describe the table of the given " + name + "."});
    output_table.add_row(
        {string() + c_command_prefix + c_list_command + c_db_subcommand,
         optionalize(pattern), "List databases optionally filtering by the " + pattern + "."});
    output_table.add_row(
        {string() + c_command_prefix + c_list_command + c_field_subcommand,
         optionalize(pattern), "List data fields optionally filtering by the " + pattern + "."});
    output_table.add_row(
        {string() + c_command_prefix + c_list_command + c_relationship_subcommand,
         optionalize(pattern), "List relationships optionally filtering by the " + pattern + "."});
    output_table.add_row(
        {string() + c_command_prefix + c_list_command + optionalize(c_table_subcommand),
         optionalize(pattern), "List tables optionally filtering by the " + pattern + "."});
#ifdef DEBUG
    // Hide FlatBuffers related commands in release build.
    output_table.add_row(
        {string() + c_command_prefix + c_generate_command + optionalize(c_db_subcommand),
         name, "Generate fbs for a given database."});
    output_table.add_row(
        {string() + c_command_prefix + c_generate_command + c_table_subcommand,
         name, "Generate fbs for a given table."});
#endif
    output_table.add_row(
        {string() + c_command_prefix + c_help_command, "", "Print help information."});
    output_table.add_row(
        {string() + c_command_prefix + c_exit_command, "", "Quit."});

    output_table.format().hide_border();
    output_table.print(ss);
    return ss.str();
}

} // namespace

bool handle_meta_command(const string& cmd)
{
    retail_assert(!cmd.empty(), "Meta command should not be empty.");
    retail_assert(
        cmd[c_cmd_prefix_index] == c_command_prefix,
        "Meta command should start with a '" + string(1, c_command_prefix) + "'.");

    if (cmd.length() < c_cmd_minimum_length)
    {
        throw invalid_command(cmd);
    }
    switch (cmd[c_command_index])
    {
    case c_list_command:
        handle_list_command(cmd);
        break;
    case c_describe_command:
        handle_describe_command(cmd);
        break;
#ifdef DEBUG
    // Hide FlatBuffers related commands in release build.
    case c_generate_command:
        handle_generate_command(cmd);
        break;
#endif
    case c_help_command:
        cout << command_usage() << endl;
        break;
    case c_exit_command:
        return false;
    default:
        throw gaia_exception("Invalid command '" + string(cmd) + "'.");
    }
    return true;
}
