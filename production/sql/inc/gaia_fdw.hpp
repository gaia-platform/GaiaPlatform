/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fstream>

// Gaia storage engine implementation
#include "storage_engine.hpp"
#include "helpers.hpp"

// all Postgres headers and function declarations must have C linkage
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
// for FDW helpers: https://www.postgresql.org/docs/devel/fdw-helpers.html
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

/*
 * Module initialization function
 */
extern void _PG_init();
/*
 * Module unload function
 */
extern void _PG_fini();

/*
 * SQL functions
 */
extern Datum gaia_fdw_handler(PG_FUNCTION_ARGS);
extern Datum gaia_fdw_validator(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(gaia_fdw_handler);
PG_FUNCTION_INFO_V1(gaia_fdw_validator);

/*
 * FDW callback routines
 */
void gaiaGetForeignRelSize(PlannerInfo *root, RelOptInfo *baserel,
                           Oid foreigntableid);
void gaiaGetForeignPaths(PlannerInfo *root, RelOptInfo *baserel,
                         Oid foreigntableid);
ForeignScan *gaiaGetForeignPlan(PlannerInfo *root, RelOptInfo *foreignrel,
                                Oid foreigntableid, ForeignPath *best_path,
                                List *tlist, List *scan_clauses,
                                Plan *outer_plan);
void gaiaBeginForeignScan(ForeignScanState *node, int eflags);
TupleTableSlot *gaiaIterateForeignScan(ForeignScanState *node);
void gaiaReScanForeignScan(ForeignScanState *node);
void gaiaEndForeignScan(ForeignScanState *node);
void gaiaAddForeignUpdateTargets(Query *parsetree, RangeTblEntry *target_rte,
                                 Relation target_relation);
List *gaiaPlanForeignModify(PlannerInfo *root, ModifyTable *plan,
                            Index resultRelation, int subplan_index);
void gaiaBeginForeignModify(ModifyTableState *mtstate,
                            ResultRelInfo *resultRelInfo, List *fdw_private,
                            int subplan_index, int eflags);
TupleTableSlot *gaiaExecForeignInsert(EState *estate,
                                      ResultRelInfo *resultRelInfo,
                                      TupleTableSlot *slot,
                                      TupleTableSlot *planSlot);
TupleTableSlot *gaiaExecForeignUpdate(EState *estate,
                                      ResultRelInfo *resultRelInfo,
                                      TupleTableSlot *slot,
                                      TupleTableSlot *planSlot);
TupleTableSlot *gaiaExecForeignDelete(EState *estate,
                                      ResultRelInfo *resultRelInfo,
                                      TupleTableSlot *slot,
                                      TupleTableSlot *planSlot);
void gaiaEndForeignModify(EState *estate, ResultRelInfo *resultRelInfo);
void gaiaBeginForeignInsert(ModifyTableState *mtstate,
                            ResultRelInfo *resultRelInfo);
void gaiaEndForeignInsert(EState *estate, ResultRelInfo *resultRelInfo);
int gaiaIsForeignRelUpdatable(Relation rel);
bool gaiaPlanDirectModify(PlannerInfo *root, ModifyTable *plan,
                          Index resultRelation, int subplan_index);
void gaiaBeginDirectModify(ForeignScanState *node, int eflags);
TupleTableSlot *gaiaIterateDirectModify(ForeignScanState *node);
void gaiaEndDirectModify(ForeignScanState *node);
void gaiaExplainForeignScan(ForeignScanState *node, struct ExplainState *es);
void gaiaExplainForeignModify(ModifyTableState *mtstate, ResultRelInfo *rinfo,
                              List *fdw_private, int subplan_index,
                              struct ExplainState *es);
void gaiaExplainDirectModify(ForeignScanState *node, struct ExplainState *es);
bool gaiaAnalyzeForeignTable(Relation relation, AcquireSampleRowsFunc *func,
                             BlockNumber *totalpages);
List *gaiaImportForeignSchema(ImportForeignSchemaStmt *stmt, Oid serverOid);
void gaiaGetForeignJoinPaths(PlannerInfo *root, RelOptInfo *joinrel,
                             RelOptInfo *outerrel, RelOptInfo *innerrel,
                             JoinType jointype, JoinPathExtraData *extra);
bool gaiaRecheckForeignScan(ForeignScanState *node, TupleTableSlot *slot);
void gaiaGetForeignUpperPaths(PlannerInfo *root, UpperRelationKind stage,
                              RelOptInfo *input_rel, RelOptInfo *output_rel,
                              void *extra);
bool gaiaRecheckForeignScan(ForeignScanState *node, TupleTableSlot *slot);
RowMarkType gaiaGetForeignRowMarkType(RangeTblEntry *rte,
                                      LockClauseStrength strength);
void gaiaRefetchForeignRow(EState *estate, ExecRowMark *erm, Datum rowid,
                           TupleTableSlot *slot, bool *updated);

/*
 * structures used by the FDW
 */

typedef void (*OptionHandler)(const char *name, const char *value, Oid context);
/*
 * Describes the valid options for objects that use this wrapper.
 */
typedef struct {
    const char *name;
    Oid context; /* Oid of catalog in which option may appear */
    OptionHandler handler;
} gaiaFdwOption;

static void handleResetStorageEngine(const char *name, const char *value,
                                     Oid context);

/*
 * Valid options for gaia_fdw.
 */
static const gaiaFdwOption valid_options[] = {
    /* Data source options */
    {"reset", ForeignServerRelationId, handleResetStorageEngine},
    /* Sentinel */
    {NULL, InvalidOid, NULL}};

/*
 * The scan state is set up in gaiaBeginForeignScan and stashed away in
 * node->fdw_private and fetched in gaiaIterateForeignScan.
 */
typedef struct {
    RootObjectDeserializer deserializer;
    // flatbuffer accessor functions indexed by attrnum
    AttributeAccessor *indexed_accessors;
    // discriminant for the following union
    bool gaia_type_is_edge;
    // the COW-SE smart ptr we are currently iterating over
    union {
        gaia::db::gaia_ptr<gaia::db::gaia_se_node> cur_node;
        gaia::db::gaia_ptr<gaia::db::gaia_se_edge> cur_edge;
    };
} gaiaFdwScanState;

/*
 * The modify state is for maintaining state of modify operations.
 *
 * It is set up in gaiaBeginForeignModify and stashed in
 * rinfo->ri_FdwState and subsequently used in gaiaExecForeignInsert,
 * gaiaExecForeignUpdate, gaiaExecForeignDelete and
 * gaiaEndForeignModify.
 */
typedef struct {
    BuilderInitializer initializer;
    BuilderFinalizer finalizer;
    // flatbuffer attribute builder functions indexed by attrnum
    AttributeBuilder *indexed_builders;
    // flatbuffers builder for INSERT and UPDATE
    flatcc_builder_t builder;
    // 0-based index of gaia_id attribute in tuple descriptor
    int pk_attr_idx;
    // 0-based index of gaia_src_id attribute in tuple descriptor (edge types
    // only)
    int src_attr_idx;
    // 0-based index of gaia_dst_id attribute in tuple descriptor (edge types
    // only)
    int dst_attr_idx;
    gaia_type_t gaia_type_id;
    // discriminant for the following union
    bool gaia_type_is_edge;
    // the COW-SE smart ptr that is the target of our update
    union {
        gaia::db::gaia_ptr<gaia::db::gaia_se_node> target_node;
        gaia::db::gaia_ptr<gaia::db::gaia_se_edge> target_edge;
    };
} gaiaFdwModifyState;

} // extern "C"
