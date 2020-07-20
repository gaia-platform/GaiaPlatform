/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fstream>

#include "array_size.hpp"
#include "gaia_db.hpp"
#include "gaia_ptr.hpp"

// All Postgres headers and function declarations must have C linkage.
extern "C" {

#include "airport_demo_type_mapping.hpp"
#include "system_catalog_type_mapping.hpp"

#include "flatbuffers_common_builder.h"

#include "postgres.h"

#include "access/reloptions.h"
#include "access/table.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "executor/executor.h"
#include "foreign/fdwapi.h"
// For FDW helpers: https://www.postgresql.org/docs/devel/fdw-helpers.html.
#include "foreign/foreign.h"
#include "nodes/makefuncs.h"
#include "nodes/pathnodes.h"
#include "optimizer/optimizer.h"
#include "optimizer/pathnode.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"

// Module initialization function.
extern void _PG_init();
// Module unload function.
extern void _PG_fini();

// SQL functions.
extern Datum gaia_fdw_handler(PG_FUNCTION_ARGS);
extern Datum gaia_fdw_validator(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(gaia_fdw_handler);
PG_FUNCTION_INFO_V1(gaia_fdw_validator);

// FDW callback routines.
void gaia_get_foreign_rel_size(
    PlannerInfo *root,
    RelOptInfo *base_rel,
    Oid foreign_table_id);

void gaia_get_foreign_paths(
    PlannerInfo *root,
    RelOptInfo *base_rel,
    Oid foreign_table_id);

ForeignScan *gaia_get_foreign_plan(
    PlannerInfo *root,
    RelOptInfo *foreign_rel,
    Oid foreign_table_id,
    ForeignPath *best_path,
    List *tlist,
    List *scan_clauses,
    Plan *outer_plan);

void gaia_begin_foreign_scan(
    ForeignScanState *node,
    int eflags);

TupleTableSlot *gaia_iterate_foreign_scan(ForeignScanState *node);

void gaia_rescan_foreign_scan(ForeignScanState *node);

void gaia_end_foreign_scan(ForeignScanState *node);

void gaia_add_foreign_update_targets(
    Query *parse_tree,
    RangeTblEntry *target_rte,
    Relation target_relation);

List *gaia_plan_foreign_modify(
    PlannerInfo *root,
    ModifyTable *plan,
    Index result_rel,
    int subplan_index);

void gaia_begin_foreign_modify(
    ModifyTableState *mtstate,
    ResultRelInfo *result_rel_info,
    List *fdw_private,
    int subplan_index,
    int eflags);

TupleTableSlot *gaia_exec_foreign_insert(
    EState *estate,
    ResultRelInfo *result_rel_info,
    TupleTableSlot *slot,
    TupleTableSlot *plan_slot);

TupleTableSlot *gaia_exec_foreign_update(
    EState *estate,
    ResultRelInfo *result_rel_info,
    TupleTableSlot *slot,
    TupleTableSlot *plan_slot);

TupleTableSlot *gaia_exec_foreign_delete(
    EState *estate,
    ResultRelInfo *result_rel_info,
    TupleTableSlot *slot,
    TupleTableSlot *plan_slot);

void gaia_end_foreign_modify(EState *estate, ResultRelInfo *result_rel_info);

void gaia_begin_foreign_insert(
    ModifyTableState *mtstate,
    ResultRelInfo *result_rel_info);

void gaia_end_foreign_insert(EState *estate, ResultRelInfo *result_rel_info);

int gaia_is_foreign_rel_updatable(Relation relation);

bool gaia_plan_direct_modify(
    PlannerInfo *root,
    ModifyTable *plan,
    Index result_rel,
    int subplan_index);

void gaia_begin_direct_modify(ForeignScanState *node, int eflags);

TupleTableSlot *gaia_iterate_direct_modify(ForeignScanState *node);

void gaia_end_direct_modify(ForeignScanState *node);

void gaia_explain_foreign_scan(ForeignScanState *node, struct ExplainState *es);

void gaia_explain_foreign_modify(
    ModifyTableState *mtstate,
    ResultRelInfo *rinfo,
    List *fdw_private,
    int subplan_index,
    struct ExplainState *es);

void gaia_explain_direct_modify(ForeignScanState *node, struct ExplainState *es);

bool gaia_analyze_foreign_table(
    Relation relation,
    AcquireSampleRowsFunc *func,
    BlockNumber *total_pages);

List *gaia_import_foreign_schema(ImportForeignSchemaStmt *stmt, Oid server_oid);

void gaia_get_foreign_join_paths(
    PlannerInfo *root,
    RelOptInfo *join_rel,
    RelOptInfo *outer_rel,
    RelOptInfo *inner_rel,
    JoinType join_type,
    JoinPathExtraData *extra);

bool gaia_recheck_foreign_scan(ForeignScanState *node, TupleTableSlot *slot);

void gaia_get_foreign_upper_paths(
    PlannerInfo *root,
    UpperRelationKind stage,
    RelOptInfo *input_rel,
    RelOptInfo *output_rel,
    void *extra);

RowMarkType gaia_get_foreign_row_mark_type(
    RangeTblEntry *rte,
    LockClauseStrength strength);

void gaia_refetch_foreign_row(
    EState *estate,
    ExecRowMark *erm,
    Datum rowid,
    TupleTableSlot *slot,
    bool *updated);

// Structures used by the FDW.
typedef void (*option_handler_fn)(const char *name, const char *value, Oid context);

// Describes the valid options for objects that use this wrapper.
typedef struct {
    const char *name;

    // Oid of catalog in which option may appear.
    Oid context;

    option_handler_fn handler;
} gaia_fdw_option_t;

// Valid options for gaia_fdw.
static const gaia_fdw_option_t valid_options[] = {
    // Sentinel.
    {NULL, InvalidOid, NULL}};

// The scan state is set up in gaia_begin_foreign_scan and stashed away in
// node->fdw_private and fetched in gaia_iterate_foreign_scan.
typedef struct {
    root_object_deserializer_fn deserializer;

    // flatbuffer accessor functions indexed by attrnum.
    attribute_accessor_fn *indexed_accessors;

    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr cur_node;
} gaia_fdw_scan_state_t;

// The modify state is for maintaining state of modify operations.
//
// It is set up in gaiaBeginForeignModify and stashed in
// rinfo->ri_FdwState and subsequently used in gaiaExecForeignInsert,
// gaiaExecForeignUpdate, gaiaExecForeignDelete and
// gaiaEndForeignModify.
typedef struct {
    builder_initializer_fn initializer;
    builder_finalizer_fn finalizer;

    // flatbuffer attribute builder functions indexed by attrnum.
    attribute_builder_fn *indexed_builders;

    // flatbuffers builder for INSERT and UPDATE.
    flatcc_builder_t builder;

    // 0-based index of gaia_id attribute in tuple descriptor.
    int pk_attr_idx;

    // 0-based index of gaia_src_id attribute in tuple descriptor (edge types
    // only).
    int src_attr_idx;

    // 0-based index of gaia_dst_id attribute in tuple descriptor (edge types
    // only).
    int dst_attr_idx;

    gaia_type_t gaia_type_id;

    // The COW-SE smart ptr that is the target of our update.
    gaia::db::gaia_ptr target_node;
} gaia_fdw_modify_state_t;

}  // extern "C"
