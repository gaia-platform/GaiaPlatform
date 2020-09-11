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

void hex_dump16(uint32_t offset, void* b, int len) {
    printf("      %08x: ", offset);
    int eight = 0;
    uint8_t* bp = (uint8_t*)b;
    int l = len;
    if (l > 16)
        l = 16;
    while (l--) {
        if (eight == 8)
            printf(" ");
        eight++;
        printf("%02x ", *bp++);
    }
    if (eight <= 8)
        printf(" ");
    while (eight++ < 16)
        printf("   ");
    printf("| ");
    l = len;
    eight = 0;
    bp = (uint8_t*)b;
    if (l > 16)
        l = 16;
    while (l--) {
        if (eight == 8)
            printf(" ");
        eight++;
        printf("%c ", *bp>=' ' && *bp<='~'? *bp: '.');
        bp++;
    }
    printf("\n");
}

void hex_dump(const char*, void* b, int len) {
    uint32_t offset = 0;
    uint8_t* bp = (uint8_t*)b;
    // printf("      %s:\n", label);
    while (len > 0) {
        hex_dump16(offset, bp, len);
        bp += 16;
        len -= 16;
        offset += 16;
    }
}


void dump_node(gaia_ptr& node_ptr, bool references, bool payload) {
    size_t num_references = node_ptr.num_references();
    printf("id=%016lx, type=%08x", node_ptr.id(), node_ptr.type());
    printf(", payload=%04zx, references=%02zx\n", node_ptr.data_size(), num_references);
    if (references && num_references) {
        auto references = node_ptr.references();
        for (size_t i=0; i<num_references; i++) {
            printf("      %02zx: %016lx\n", i, references[i]);
        }
    }
    if (payload)
        hex_dump("payload", node_ptr.data(), node_ptr.data_size());
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
    for (auto i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.compare("--references") == 0)
            references = true;
        else if (arg.compare("--payload") == 0)
            payload = true;
        else if (arg.compare("--catalog") == 0)
            catalog = true;
        else if (arg.compare(0, 7, "--start") == 0) {
            if (arg.length() == 7)
                low = atoi(argv[++i]);
            else
                low = atoi(arg.substr(8).c_str());
        }
        else if (arg.compare(0, 5, "--end") == 0) {
            if (arg.length() == 5)
                high = atoi(argv[++i]);
            else
                high = atoi(arg.substr(6).c_str());
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

    for (id=low; id<=high; id++) {
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
        for (id=c_system_table_reserved_range_start; id<c_system_table_reserved_range_end; id++) {
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
