/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

// COW-SE implementation
#include "cow_se.h"
// airport demo data loader
#include "data_loader.h"
#include "helpers.h"

// all Postgres headers and function declarations must have C linkage
extern "C" {

#include "airport_demo_type_mapping.h"

#include "postgres.h"

#include "access/reloptions.h"
#include "catalog/pg_foreign_server.h"
#include "commands/defrem.h"
#include "foreign/fdwapi.h"
// for FDW helpers: https://www.postgresql.org/docs/devel/fdw-helpers.html
#include "foreign/foreign.h"
#include "optimizer/optimizer.h"
#include "optimizer/pathnode.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "utils/lsyscache.h"

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
extern Datum cow_se_fdw_handler(PG_FUNCTION_ARGS);
extern Datum cow_se_fdw_validator(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(cow_se_fdw_handler);
PG_FUNCTION_INFO_V1(cow_se_fdw_validator);

/*
 * FDW callback routines
 */
void cow_seGetForeignRelSize(PlannerInfo *root,
                                      RelOptInfo *baserel,
                                      Oid foreigntableid);
void cow_seGetForeignPaths(PlannerInfo *root,
                                    RelOptInfo *baserel,
                                    Oid foreigntableid);
ForeignScan *cow_seGetForeignPlan(PlannerInfo *root,
                                           RelOptInfo *foreignrel,
                                           Oid foreigntableid,
                                           ForeignPath *best_path,
                                           List *tlist,
                                           List *scan_clauses,
                                           Plan *outer_plan);
void cow_seBeginForeignScan(ForeignScanState *node, int eflags);
TupleTableSlot *cow_seIterateForeignScan(ForeignScanState *node);
void cow_seReScanForeignScan(ForeignScanState *node);
void cow_seEndForeignScan(ForeignScanState *node);
void cow_seAddForeignUpdateTargets(Query *parsetree,
                                            RangeTblEntry *target_rte,
                                            Relation target_relation);
List *cow_sePlanForeignModify(PlannerInfo *root,
                                       ModifyTable *plan,
                                       Index resultRelation,
                                       int subplan_index);
void cow_seBeginForeignModify(ModifyTableState *mtstate,
                                       ResultRelInfo *resultRelInfo,
                                       List *fdw_private,
                                       int subplan_index,
                                       int eflags);
TupleTableSlot *cow_seExecForeignInsert(EState *estate,
                                                 ResultRelInfo *resultRelInfo,
                                                 TupleTableSlot *slot,
                                                 TupleTableSlot *planSlot);
TupleTableSlot *cow_seExecForeignUpdate(EState *estate,
                                                 ResultRelInfo *resultRelInfo,
                                                 TupleTableSlot *slot,
                                                 TupleTableSlot *planSlot);
TupleTableSlot *cow_seExecForeignDelete(EState *estate,
                                                 ResultRelInfo *resultRelInfo,
                                                 TupleTableSlot *slot,
                                                 TupleTableSlot *planSlot);
void cow_seEndForeignModify(EState *estate,
                                     ResultRelInfo *resultRelInfo);
void cow_seBeginForeignInsert(ModifyTableState *mtstate,
                                       ResultRelInfo *resultRelInfo);
void cow_seEndForeignInsert(EState *estate,
                                     ResultRelInfo *resultRelInfo);
int  cow_seIsForeignRelUpdatable(Relation rel);
bool cow_sePlanDirectModify(PlannerInfo *root,
                                     ModifyTable *plan,
                                     Index resultRelation,
                                     int subplan_index);
void cow_seBeginDirectModify(ForeignScanState *node, int eflags);
TupleTableSlot *cow_seIterateDirectModify(ForeignScanState *node);
void cow_seEndDirectModify(ForeignScanState *node);
void cow_seExplainForeignScan(ForeignScanState *node,
                                       struct ExplainState *es);
void cow_seExplainForeignModify(ModifyTableState *mtstate,
                                         ResultRelInfo *rinfo,
                                         List *fdw_private,
                                         int subplan_index,
                                         struct ExplainState *es);
void cow_seExplainDirectModify(ForeignScanState *node,
                                        struct ExplainState *es);
bool cow_seAnalyzeForeignTable(Relation relation,
                                        AcquireSampleRowsFunc *func,
                                        BlockNumber *totalpages);
List *cow_seImportForeignSchema(ImportForeignSchemaStmt *stmt,
                                         Oid serverOid);
void cow_seGetForeignJoinPaths(PlannerInfo *root,
                                        RelOptInfo *joinrel,
                                        RelOptInfo *outerrel,
                                        RelOptInfo *innerrel,
                                        JoinType jointype,
                                        JoinPathExtraData *extra);
bool cow_seRecheckForeignScan(ForeignScanState *node,
                                       TupleTableSlot *slot);
void cow_seGetForeignUpperPaths(PlannerInfo *root,
                                         UpperRelationKind stage,
                                         RelOptInfo *input_rel,
                                         RelOptInfo *output_rel,
                                         void *extra);
bool cow_seRecheckForeignScan(ForeignScanState *node,
                                       TupleTableSlot *slot);
RowMarkType cow_seGetForeignRowMarkType(RangeTblEntry *rte,
                                               LockClauseStrength strength);
void cow_seRefetchForeignRow(EState *estate,
                                      ExecRowMark *erm,
                                      Datum rowid,
                                      TupleTableSlot *slot,
                                      bool *updated);

/*
 * structures used by the FDW
 */

typedef void (*OptionHandler)(const char *name, const char *value, Oid context);
/*
 * Describes the valid options for objects that use this wrapper.
 */
typedef struct {
    const char *optname;
    Oid optcontext;     /* Oid of catalog in which option may appear */
    OptionHandler opthandler; 
} cow_seFdwOption;

static void handleDataDir(const char *name, const char *value, Oid context);

/*
 * Valid options for cow_se_fdw.
 */
static const cow_seFdwOption valid_options[] = {
	/* Data source options */
	{ "data_dir", ForeignServerRelationId, handleDataDir },
    /* Sentinel */
	{ NULL, InvalidOid, NULL }
};

/*
 * The plan state is set up in cow_seGetForeignRelSize and stashed away in
 * baserel->fdw_private and fetched in cow_seGetForeignPaths.
 */
typedef struct {
    ForeignTable *table;
    ForeignServer *server;
    Bitmapset *attrs_used;
    const char *data_dir;
} cow_seFdwPlanState;

/*
 * The plan state is set up in cow_seBeginForeignScan and stashed away in
 * node->fdw_private and fetched in cow_seIterateForeignScan.
 */
typedef struct {
    RootObjectDeserializer deserializer;
    // flatbuffer accessor functions indexed by attrnum
    AttributeAccessor *indexed_accessors;
    // discriminant for the following union
    bool gaia_type_is_edge;
    // the COW-SE smart ptr we are currently iterating over
    union {
        gaia_se::gaia_ptr<gaia_se::gaia_se_node> cur_node;
        gaia_se::gaia_ptr<gaia_se::gaia_se_edge> cur_edge;
    };
} cow_seFdwScanState;

/*
 * The modify state is for maintaining state of modify operations.
 *
 * It is set up in cow_seBeginForeignModify and stashed in
 * rinfo->ri_FdwState and subsequently used in cow_seExecForeignInsert,
 * cow_seExecForeignUpdate, cow_seExecForeignDelete and
 * cow_seEndForeignModify.
 */
typedef struct
{
    gaia_se::gaia_ptr<gaia_se::gaia_se_node> target_node;
} cow_seFdwModifyState;

} // extern "C"
