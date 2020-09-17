/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include "gaia_ptr.hpp"
#include "system_table_types.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <set>

using namespace gaia::db;
using namespace std;

constexpr int c_group_size = 8;
constexpr int c_line_size = 2*c_group_size;

void hex_dump16(uint32_t offset, void* binary_buff, int binary_length) {
    printf("      %08x: ", offset);
    int line_position = 0;
    uint8_t* binary_ptr = (uint8_t*)binary_buff;
    int remaining_length = binary_length;
    if (remaining_length > c_line_size) {
        remaining_length = c_line_size;
    }
    while (remaining_length--) {
        if (line_position == c_group_size) {
            printf(" ");
        }
        line_position++;
        printf("%02x ", *binary_ptr++);
    }
    if (line_position <= c_group_size) {
        printf(" ");
    }
    while (line_position++ < c_line_size) {
        printf("   ");
    }
    printf("| ");
    remaining_length = binary_length;
    line_position = 0;
    binary_ptr = (uint8_t*)binary_buff;
    if (remaining_length > c_line_size) {
        remaining_length = c_line_size;
    }
    while (remaining_length--) {
        if (line_position == c_group_size) {
            printf(" ");
        }
        line_position++;
        printf("%c ", *binary_ptr>=' ' && *binary_ptr<='~'? *binary_ptr: '.');
        binary_ptr++;
    }
    printf("\n");
}

void hex_dump(void* binary_buff, int binary_length) {
    uint32_t offset = 0;
    uint8_t* binary_ptr = (uint8_t*)binary_buff;
    while (binary_length > 0) {
        hex_dump16(offset, binary_ptr, binary_length);
        binary_ptr += 16;
        binary_length -= 16;
        offset += 16;
    }
}


void dump_node(gaia_ptr& node_ptr, bool references, bool payload) {
    size_t num_references = node_ptr.num_references();
    printf("id=%016lx, type=%08x", node_ptr.id(), node_ptr.type());
    printf(", payload=%04zx, references=%02zx\n", node_ptr.data_size(), num_references);
    if (references && num_references) {
        auto references = node_ptr.references();
        for (size_t i = 0; i < num_references; i++) {
            printf("      %02zx: %016lx\n", i, references[i]);
        }
    }
    if (payload) {
        hex_dump(node_ptr.data(), node_ptr.data_size());
    }
}

int main(int argc, char *argv[]) {
    gaia_id_t id, low, high;
    bool payload = false;
    bool references = false;
    bool catalog = false;
    high = c_system_table_reserved_range_end;
    low = 1;

    // Usage:
    //  gaia_dump --start=ID --end=ID --references --payload --catalog
    for (auto i = 1; i < argc; i++) {
        string arg(argv[i]);
        if (arg.compare("--references") == 0) {
            references = true;
        } else if (arg.compare("--payload") == 0) {
            payload = true;
        } else if (arg.compare("--catalog") == 0) {
            catalog = true;
        } else if (arg.compare(0, 7, "--start") == 0) {
            if (arg.length() == 7) {
                low = atoi(argv[++i]);
            } else {
                low = atoi(arg.substr(8).c_str());
            }
        }
        else if (arg.compare(0, 5, "--end") == 0) {
            if (arg.length() == 5) {
                high = atoi(argv[++i]);
            } else {
                high = atoi(arg.substr(6).c_str());
            }
        }
        else {
            fprintf(stderr, "Invalid command-line option: %s\n", argv[i]);
            exit(1);
        }
    }

    try {
        gaia::db::begin_session();
    }
    catch (gaia_exception& e) {
        fprintf(stderr, "Startup failure, exception: %s\n", e.what());
        exit(1);
    }

    begin_transaction();

    for (id = low; id <= high; id++) {
        try {
            auto node_ptr = gaia_ptr::open(id);
            if (node_ptr) {
                dump_node(node_ptr, references, payload);
            }
            else {
                // printf("nothing at %lu\n", id);
            }
        }
        catch (gaia_exception& e) {
            fprintf(stderr, "exception \"%s\"\n", e.what());
        }
    }
    // Catalog is optional.
    if (catalog) {
        for (id = c_system_table_reserved_range_start; id <= c_system_table_reserved_range_end; id++) {
            try {
                auto node_ptr = gaia_ptr::open(id);
                if (node_ptr) {
                    dump_node(node_ptr, references, payload);
                }
            }
            catch (gaia_exception& e) {
                fprintf(stderr, "exception \"%s\"\n", e.what());
            }
        }
    }
    commit_transaction();

    gaia::db::end_session();
}
