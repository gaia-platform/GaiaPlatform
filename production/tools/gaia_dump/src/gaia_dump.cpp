/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include "gaia_dump.hpp"

#include <memory>
#include <vector>

#include "gaia_db.hpp"
#include "system_table_types.hpp"

constexpr int c_group_size = 8;
constexpr int c_line_size = 2 * c_group_size;
constexpr int c_line_length = 300;
constexpr int c_bytes_per_line = 16;

using namespace gaia::db;

// One printable line of output.
static char g_longstring[c_line_length];

string hex_dump16(uint32_t offset, void* binary_buff, int binary_length)
{
    string dump;

    sprintf(g_longstring, "      %08x: ", offset);
    dump += g_longstring;
    int line_position = 0;
    auto binary_ptr = static_cast<uint8_t*>(binary_buff);
    int remaining_length = binary_length;
    if (remaining_length > c_line_size)
    {
        remaining_length = c_line_size;
    }
    while (remaining_length--)
    {
        if (line_position == c_group_size)
        {
            dump += " ";
        }
        line_position++;
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        sprintf(g_longstring, "%02x ", *binary_ptr++);
        dump += g_longstring;
    }

    if (line_position <= c_group_size)
    {
        dump += " ";
    }
    while (line_position++ < c_line_size)
    {
        dump += "   ";
    }
    dump += "| ";
    remaining_length = binary_length;
    line_position = 0;
    binary_ptr = static_cast<uint8_t*>(binary_buff);
    if (remaining_length > c_line_size)
    {
        remaining_length = c_line_size;
    }
    while (remaining_length--)
    {
        if (line_position == c_group_size)
        {
            dump += " ";
        }
        line_position++;
        sprintf(g_longstring, "%c ", *binary_ptr >= ' ' && *binary_ptr <= '~' ? *binary_ptr : '.');
        dump += g_longstring;
        binary_ptr++;
    }
    dump += "\n";
    return dump;
}

static string hex_dump(void* binary_buff, int binary_length, int& line_limit)
{
    string dump;
    uint32_t offset = 0;
    auto binary_ptr = static_cast<uint8_t*>(binary_buff);
    while (binary_length > 0)
    {
        dump += hex_dump16(offset, binary_ptr, binary_length);
        if (--line_limit == 0)
        {
            return dump;
        }
        binary_ptr += c_bytes_per_line;
        binary_length -= c_bytes_per_line;
        offset += c_bytes_per_line;
    }
    return dump;
}

static string dump_node(gaia_ptr& node_ptr, bool references, bool payload, int& line_limit)
{
    string dump;
    size_t num_references = node_ptr.num_references();
    sprintf(g_longstring, "id=%016lx, type=%08x, payload=%04zx, references=%02zx\n", node_ptr.id(), node_ptr.type(), node_ptr.data_size(), num_references);
    dump += g_longstring;
    if (--line_limit == 0)
    {
        return dump;
    }
    if (references && num_references)
    {
        auto references = node_ptr.references();
        for (size_t i = 0; i < num_references; i++)
        {
            sprintf(g_longstring, "      %02zx: %016lx\n", i, references[i]);
            dump += g_longstring;
            if (--line_limit == 0)
            {
                return dump;
            }
        }
    }
    if (payload)
    {
        dump += hex_dump(node_ptr.data(), node_ptr.data_size(), line_limit);
        if (line_limit == 0)
        {
            return dump;
        }
    }
    return dump;
}

string gaia_dump(gaia_id_t low, gaia_id_t high, bool payload, bool references, bool catalog, int& line_limit)
{
    gaia_id_t id;
    string dump;

    begin_transaction();

    for (id = low; id <= high; id++)
    {
        try
        {
            auto node_ptr = gaia_ptr::open(id);
            if (node_ptr)
            {
                // If 'catalog' is true, don't check the catalog range.
                if (catalog || node_ptr.type() < c_system_table_reserved_range_start)
                {
                    dump += dump_node(node_ptr, references, payload, line_limit);
                    if (line_limit == 0)
                    {
                        break;
                    }
                }
            }
        }
        catch (gaia_exception& e)
        {
            fprintf(stderr, "exception \"%s\"\n", e.what());
        }
    }
    commit_transaction();

    return dump;
}
