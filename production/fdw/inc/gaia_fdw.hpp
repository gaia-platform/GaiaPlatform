////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

// All Postgres headers and function declarations must have C linkage.
extern "C"
{

// postgres.h must be included prior to other headers.
#include <postgres.h>
// These surrounding comments will ensure that the header order is not changed by clang-format.
// For FDW helpers: https://www.postgresql.org/docs/devel/fdw-helpers.html.
#include <access/reloptions.h>
#include <access/table.h>
#include <catalog/pg_foreign_server.h>
#include <catalog/pg_type.h>
#include <commands/defrem.h>
#include <executor/executor.h>
#include <foreign/fdwapi.h>
#include <foreign/foreign.h>
#include <nodes/makefuncs.h>
#include <nodes/pathnodes.h>
#include <optimizer/optimizer.h>
#include <optimizer/pathnode.h>
#include <optimizer/planmain.h>
#include <optimizer/restrictinfo.h>
#include <parser/parsetree.h>
#include <utils/lsyscache.h>
#include <utils/rel.h>
#include <utils/syscache.h>
#if defined(PG_MAJORVERSION_NUM) && PG_MAJORVERSION_NUM >= 14
#include "optimizer/appendinfo.h"
#endif

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
        PlannerInfo* root,
        RelOptInfo* base_rel,
        Oid foreign_table_id);

    void gaia_get_foreign_paths(
        PlannerInfo* root,
        RelOptInfo* base_rel,
        Oid foreign_table_id);

    ForeignScan* gaia_get_foreign_plan(
        PlannerInfo* root,
        RelOptInfo* foreign_rel,
        Oid foreign_table_id,
        ForeignPath* best_path,
        List* tlist,
        List* scan_clauses,
        Plan* outer_plan);

    void gaia_begin_foreign_scan(
        ForeignScanState* node,
        int eflags);

    TupleTableSlot* gaia_iterate_foreign_scan(ForeignScanState* node);

    void gaia_rescan_foreign_scan(ForeignScanState* node);

    void gaia_end_foreign_scan(ForeignScanState* node);

#if defined(PG_MAJORVERSION_NUM) && PG_MAJORVERSION_NUM >= 14
    void gaia_add_foreign_update_targets(
        PlannerInfo* root,
        Index rtindex,
        RangeTblEntry* target_rte,
        Relation target_relation);
#else
    void gaia_add_foreign_update_targets(
        Query* parse_tree,
        RangeTblEntry* target_rte,
        Relation target_relation);
#endif

    List* gaia_plan_foreign_modify(
        PlannerInfo* root,
        ModifyTable* plan,
        Index result_rel,
        int subplan_index);

    void gaia_begin_foreign_modify(
        ModifyTableState* mtstate,
        ResultRelInfo* result_rel_info,
        List* fdw_private,
        int subplan_index,
        int eflags);

    TupleTableSlot* gaia_exec_foreign_insert(
        EState* estate,
        ResultRelInfo* result_rel_info,
        TupleTableSlot* slot,
        TupleTableSlot* plan_slot);

    TupleTableSlot* gaia_exec_foreign_update(
        EState* estate,
        ResultRelInfo* result_rel_info,
        TupleTableSlot* slot,
        TupleTableSlot* plan_slot);

    TupleTableSlot* gaia_exec_foreign_delete(
        EState* estate,
        ResultRelInfo* result_rel_info,
        TupleTableSlot* slot,
        TupleTableSlot* plan_slot);

    void gaia_end_foreign_modify(EState* estate, ResultRelInfo* result_rel_info);

    void gaia_begin_foreign_insert(
        ModifyTableState* mtstate,
        ResultRelInfo* result_rel_info);

    void gaia_end_foreign_insert(EState* estate, ResultRelInfo* result_rel_info);

    int gaia_is_foreign_rel_updatable(Relation relation);

    bool gaia_plan_direct_modify(
        PlannerInfo* root,
        ModifyTable* plan,
        Index result_rel,
        int subplan_index);

    void gaia_begin_direct_modify(ForeignScanState* node, int eflags);

    TupleTableSlot* gaia_iterate_direct_modify(ForeignScanState* node);

    void gaia_end_direct_modify(ForeignScanState* node);

    void gaia_explain_foreign_scan(ForeignScanState* node, struct ExplainState* es);

    void gaia_explain_foreign_modify(
        ModifyTableState* mtstate,
        ResultRelInfo* rinfo,
        List* fdw_private,
        int subplan_index,
        struct ExplainState* es);

    void gaia_explain_direct_modify(ForeignScanState* node, struct ExplainState* es);

    bool gaia_analyze_foreign_table(
        Relation relation,
        AcquireSampleRowsFunc* func,
        BlockNumber* total_pages);

    List* gaia_import_foreign_schema(ImportForeignSchemaStmt* stmt, Oid server_oid);

    void gaia_get_foreign_join_paths(
        PlannerInfo* root,
        RelOptInfo* join_rel,
        RelOptInfo* outer_rel,
        RelOptInfo* inner_rel,
        JoinType join_type,
        JoinPathExtraData* extra);

    bool gaia_recheck_foreign_scan(ForeignScanState* node, TupleTableSlot* slot);

    void gaia_get_foreign_upper_paths(
        PlannerInfo* root,
        UpperRelationKind stage,
        RelOptInfo* input_rel,
        RelOptInfo* output_rel,
        void* extra);

    RowMarkType gaia_get_foreign_row_mark_type(
        RangeTblEntry* rte,
        LockClauseStrength strength);

    void gaia_refetch_foreign_row(
        EState* estate,
        ExecRowMark* erm,
        Datum rowid,
        TupleTableSlot* slot,
        bool* updated);

} // extern "C"
