/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "cow_se_fdw.h"

using namespace std;
using namespace gaia_se;

// magic block for extension library
extern "C" {
    PG_MODULE_MAGIC;
}

/*
 * The FDW handler function returns a palloc'd FdwRoutine struct containing
 * pointers to the callback functions that will be called by the planner,
 * executor, and various maintenance commands. The scan-related functions are
 * required, the rest are optional.
*/
extern "C"
Datum
cow_se_fdw_handler(PG_FUNCTION_ARGS)
{
    elog(DEBUG1, "entering function %s", __func__);

    FdwRoutine *routine = makeNode(FdwRoutine);

    /* Functions for scanning foreign tables */
    routine->GetForeignRelSize = cow_seGetForeignRelSize;
    routine->GetForeignPaths = cow_seGetForeignPaths;
    routine->GetForeignPlan = cow_seGetForeignPlan;
    routine->BeginForeignScan = cow_seBeginForeignScan;
    routine->IterateForeignScan = cow_seIterateForeignScan;
    routine->ReScanForeignScan = cow_seReScanForeignScan;
    routine->EndForeignScan = cow_seEndForeignScan;

    // /* Functions for updating foreign tables */
    // routine->AddForeignUpdateTargets = cow_seAddForeignUpdateTargets;
    // routine->PlanForeignModify = cow_sePlanForeignModify;
    // routine->BeginForeignModify = cow_seBeginForeignModify;
    // routine->ExecForeignInsert = cow_seExecForeignInsert;
    // routine->ExecForeignUpdate = cow_seExecForeignUpdate;
    // routine->ExecForeignDelete = cow_seExecForeignDelete;
    // routine->EndForeignModify = cow_seEndForeignModify;
    // routine->BeginForeignInsert = cow_seBeginForeignInsert;
    // routine->EndForeignInsert = cow_seEndForeignInsert;
    // routine->IsForeignRelUpdatable = cow_seIsForeignRelUpdatable;
    // routine->PlanDirectModify = cow_sePlanDirectModify;
    // routine->BeginDirectModify = cow_seBeginDirectModify;
    // routine->IterateDirectModify = cow_seIterateDirectModify;
    // routine->EndDirectModify = cow_seEndDirectModify;

    // /* Function for EvalPlanQual rechecks */
    // routine->RecheckForeignScan = cow_seRecheckForeignScan;
    // /* Support functions for EXPLAIN */
    // routine->ExplainForeignScan = cow_seExplainForeignScan;
    // routine->ExplainForeignModify = cow_seExplainForeignModify;
    // routine->ExplainDirectModify = cow_seExplainDirectModify;

    // /* Support functions for ANALYZE */
    // routine->AnalyzeForeignTable = cow_seAnalyzeForeignTable;

    // /* Support functions for IMPORT FOREIGN SCHEMA */
    // routine->ImportForeignSchema = cow_seImportForeignSchema;

    // /* Support functions for join push-down */
    // routine->GetForeignJoinPaths = cow_seGetForeignJoinPaths;

    // /* Support functions for upper relation push-down */
    // routine->GetForeignUpperPaths = cow_seGetForeignUpperPaths;

    // /* Support functions for late row locking */
    // routine->RecheckForeignScan = cow_seRecheckForeignScan;
    // routine->GetForeignRowMarkType = cow_seGetForeignRowMarkType;
    // routine->RefetchForeignRow = cow_seRefetchForeignRow;

    PG_RETURN_POINTER(routine);
}

/*
 * The validator function is responsible for validating options given in CREATE
 * and ALTER commands for its foreign data wrapper, as well as foreign servers,
 * user mappings, and foreign tables using the wrapper. The validator function
 * must be registered as taking two arguments, a text array containing the
 * options to be validated, and an OID representing the type of object the
 * options are associated with (in the form of the OID of the system catalog
 * the object would be stored in, either ForeignDataWrapperRelationId,
 * ForeignServerRelationId, UserMappingRelationId, or ForeignTableRelationId).
 * If no validator function is supplied, options are not checked at object
 * creation time or object alteration time.
 */
extern "C"
Datum
cow_se_fdw_validator(PG_FUNCTION_ARGS)
{
    List *options_list = untransformRelOptions(PG_GETARG_DATUM(0));

    elog(DEBUG1, "entering function %s", __func__);

    /* make sure the options are valid */

    /* no options are supported */

    if (list_length(options_list) > 0)
        ereport(ERROR,
                (errcode(ERRCODE_FDW_INVALID_OPTION_NAME),
                 errmsg("invalid options"),
                 errhint("cow_se FDW does not support any options")));

    PG_RETURN_VOID();
}

// from https://github.com/adjust/parquet_fdw/blob/master/parquet_impl.cpp
static void
get_table_options(Oid relid, cow_seFdwPlanState *fdw_private)
{
    ListCell     *lc;

    foreach(lc, fdw_private->table->options)
    {
        DefElem    *def = (DefElem *) lfirst(lc);

    //     if (strcmp(def->defname, "filename") == 0)
    //         fdw_private->filename = defGetString(def);
    //     else if (strcmp(def->defname, "sorted") == 0)
    //     {
    //         fdw_private->attrs_sorted =
    //             parse_attributes_list(defGetString(def), relid);
    //     }
    //     else if (strcmp(def->defname, "use_mmap") == 0)
    //     { 
    //         if (!parse_bool(defGetString(def), &fdw_private->use_mmap))
    //             ereport(ERROR,
    //                     (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
    //                      errmsg("invalid value for boolean option \"%s\": %s",
    //                             def->defname, defGetString(def))));
    //     }
    //     else if (strcmp(def->defname, "use_threads") == 0)
    //     {
    //         if (!parse_bool(defGetString(def), &fdw_private->use_threads))
    //             ereport(ERROR,
    //                     (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
    //                      errmsg("invalid value for boolean option \"%s\": %s",
    //                             def->defname, defGetString(def))));
    //     }
    //     else
        elog(ERROR, "unknown option '%s'", def->defname);
    }
}

extern "C"
void
cow_seGetForeignRelSize(PlannerInfo *root,
                           RelOptInfo *baserel,
                           Oid foreigntableid)
{
    /*
     * Obtain relation size estimates for a foreign table. This is called at
     * the beginning of planning for a query that scans a foreign table. root
     * is the planner's global information about the query; baserel is the
     * planner's information about this table; and foreigntableid is the
     * pg_class OID of the foreign table. (foreigntableid could be obtained
     * from the planner data structures, but it's passed explicitly to save
     * effort.)
     *
     * This function should update baserel->rows to be the expected number of
     * rows returned by the table scan, after accounting for the filtering
     * done by the restriction quals. The initial value of baserel->rows is
     * just a constant default estimate, which should be replaced if at all
     * possible. The function may also choose to update baserel->width if it
     * can compute a better estimate of the average result row width.
     */

    elog(DEBUG1, "entering function %s", __func__);

    baserel->rows = 0;

    // we allocate the whole plan state here but only fill in the base state
    cow_seFdwPlanState *plan_state = (cow_seFdwPlanState *) palloc0(sizeof(cow_seFdwPlanState));
    baserel->fdw_private = (void *) plan_state;

    // initialize required state in plan_state
    plan_state->table = GetForeignTable(foreigntableid);
    plan_state->server = GetForeignServer(plan_state->table->serverid);

    // // cache conds in plan_state for later analysis
    // plan_state->conds = baserel->baserestrictinfo;
    // foreach(lc, baserel->baserestrictinfo)
    // {
    //     RestrictInfo *ri = (RestrictInfo *) lfirst(lc);
    //     plan_state->conds = lappend(plan_state->conds, ri);
    // }

    get_table_options(foreigntableid, plan_state);

}

// from https://github.com/adjust/parquet_fdw/blob/master/parquet_impl.cpp
static void
extract_used_attributes(RelOptInfo *baserel)
{
    cow_seFdwPlanState *fdw_private = (cow_seFdwPlanState *) baserel->fdw_private;
    ListCell *lc;

    pull_varattnos((Node *) baserel->reltarget->exprs,
                   baserel->relid,
                   &fdw_private->attrs_used);

    foreach(lc, baserel->baserestrictinfo)
    {
        RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);

        pull_varattnos((Node *) rinfo->clause,
                       baserel->relid,
                       &fdw_private->attrs_used);
    }

    if (bms_is_empty(fdw_private->attrs_used))
    {
        bms_free(fdw_private->attrs_used);
        fdw_private->attrs_used = bms_make_singleton(1 - FirstLowInvalidHeapAttributeNumber);
    }
}

extern "C"
void
cow_seGetForeignPaths(PlannerInfo *root,
                         RelOptInfo *baserel,
                         Oid foreigntableid)
{
    /*
     * Create possible access paths for a scan on a foreign table. This is
     * called during query planning. The parameters are the same as for
     * GetForeignRelSize, which has already been called.
     *
     * This function must generate at least one access path (ForeignPath node)
     * for a scan on the foreign table and must call add_path to add each such
     * path to baserel->pathlist. It's recommended to use
     * create_foreignscan_path to build the ForeignPath nodes. The function
     * can generate multiple access paths, e.g., a path which has valid
     * pathkeys to represent a pre-sorted result. Each access path must
     * contain cost estimates, and can contain any FDW-private information
     * that is needed to identify the specific scan method intended.
     */
    
    cow_seFdwPlanState *plan_state = (cow_seFdwPlanState *) baserel->fdw_private;
    Cost startup_cost, total_cost;

    elog(DEBUG1, "entering function %s", __func__);

    startup_cost = 0;
    total_cost = startup_cost + baserel->rows;

    /* Collect used attributes to reduce number of read columns during scan */
    extract_used_attributes(baserel);

    /* Create a ForeignPath node and add it as only possible path */
    add_path(baserel, (Path *)
             create_foreignscan_path(root, baserel,
                                     NULL,      /* default pathtarget */
                                     baserel->rows,
                                     startup_cost,
                                     total_cost,
                                     NIL,       /* no pathkeys */
                                     NULL,      /* no outer rel either */
                                     NULL,      /* no extra plan */
                                     NULL));    /* no per-path private info */
}


// from https://github.com/pgspider/sqlite_fdw/blob/master/deparse.c
// static List *
// build_remote_tlist(RelOptInfo *foreignrel)
// {
//     List *tlist = NIL;
//     cow_seFdwPlanState *plan_state = (cow_seFdwPlanState *) baserel->fdw_private;
//     ListCell   *lc;

//     /*
//      * We require columns specified in foreignrel->reltarget->exprs and those
//      * required for evaluating the local conditions.
//      */
//     tlist = add_to_flat_tlist(tlist,
//                               pull_var_clause((Node *) foreignrel->reltarget->exprs,
//                                               PVC_RECURSE_PLACEHOLDERS));
//     foreach(lc, plan_state->local_conds)
//     {
//         RestrictInfo *rinfo = lfirst_node(RestrictInfo, lc);

//         tlist = add_to_flat_tlist(tlist,
//                                   pull_var_clause((Node *) rinfo->clause,
//                                                   PVC_RECURSE_PLACEHOLDERS));
//     }

//     return tlist;
// }

extern "C"
ForeignScan *
cow_seGetForeignPlan(PlannerInfo *root,
                        RelOptInfo *baserel,
                        Oid foreigntableid,
                        ForeignPath *best_path,
                        List *tlist,
                        List *scan_clauses,
                        Plan *outer_plan)
{
    /*
     * Create a ForeignScan plan node from the selected foreign access path.
     * This is called at the end of query planning. The parameters are as for
     * GetForeignRelSize, plus the selected ForeignPath (previously produced
     * by GetForeignPaths), the target list to be emitted by the plan node,
     * and the restriction clauses to be enforced by the plan node.
     *
     * This function must create and return a ForeignScan plan node; it's
     * recommended to use make_foreignscan to build the ForeignScan node.
     *
     */
    
    Index scan_relid = baserel->relid;
    cow_seFdwPlanState *plan_state = (cow_seFdwPlanState *) baserel->fdw_private;
    
    elog(DEBUG1, "entering function %s", __func__);

    /*
     * We have no native ability to evaluate restriction clauses, so we just
     * put all the scan_clauses into the plan node's qual list for the
     * executor to check. So all we have to do here is strip RestrictInfo
     * nodes from the clauses and ignore pseudoconstants (which will be
     * handled elsewhere).
     */
    scan_clauses = extract_actual_clauses(scan_clauses, false);

    /*
     * We can't just pass arbitrary structure into make_foreignscan() because
     * in some cases (i.e. plan caching) postgres may want to make a copy of
     * the plan and it can only make copy of something it knows of, namely
     * Nodes. So we need to convert everything in nodes and store it in a List.
     */
    List *attrs_used = NIL;
    int col = -1;
    while ((col = bms_next_member(plan_state->attrs_used, col)) >= 0) {
        /* bit numbers are offset by FirstLowInvalidHeapAttributeNumber */
        AttrNumber attno = col + FirstLowInvalidHeapAttributeNumber;
        if (attno <= InvalidAttrNumber) { /* shouldn't happen */
            elog(ERROR, "system-column update is not supported");
        }
        attrs_used = lappend_int(attrs_used, attno);
    }

    // NB: it's probably conceptually cleaner to use a custom tlist rather than passing down attrs_used to the plan,
    // (we don't need to fill in unused columns with NULL), but building a custom tlist seems like a huge pain.
    // See build_remote_tlist() for one approach.
    List *params = list_make2(makeString(get_rel_name(plan_state->table->relid)),
                        attrs_used);

    /* Create the ForeignScan node */
    return make_foreignscan(tlist,
                            scan_clauses,
                            scan_relid,
                            NIL,    /* no expressions to evaluate */
                            params,    /* we don't use this now but might later */
                            NIL,    /* no custom tlist */
                            NIL,    /* no remote quals */
                            outer_plan);
}

extern "C"
void
cow_seBeginForeignScan(ForeignScanState *node,
                          int eflags)
{
    /*
     * Begin executing a foreign scan. This is called during executor startup.
     * It should perform any initialization needed before the scan can start,
     * but not start executing the actual scan (that should be done upon the
     * first call to IterateForeignScan). The ForeignScanState node has
     * already been created, but its fdw_state field is still NULL.
     * Information about the table to scan is accessible through the
     * ForeignScanState node (in particular, from the underlying ForeignScan
     * plan node, which contains any FDW-private information provided by
     * GetForeignPlan). eflags contains flag bits describing the executor's
     * operating mode for this plan node.
     *
     * Note that when (eflags & EXEC_FLAG_EXPLAIN_ONLY) is true, this function
     * should not perform any externally-visible actions; it should only do
     * the minimum required to make the node state valid for
     * ExplainForeignScan and EndForeignScan.
     *
     */

    elog(DEBUG1, "entering function %s", __func__);

    ForeignScan *plan = (ForeignScan *) node->ss.ps.plan;
    List *fdw_private = plan->fdw_private;

    /* Unwrap fdw_private */
    char *table_name = strVal((Value *) linitial(fdw_private));
    List *attrs_used_list = (List *) lsecond(fdw_private);

    RelationAttributeMapping mapping;
    if (strcmp(table_name, "airports") == 0) {
        mapping = AIRPORT_MAPPING;
    } else if (strcmp(table_name, "airlines") == 0) {
        mapping = AIRLINE_MAPPING;
    } else if (strcmp(table_name, "routes") == 0) {
        mapping = ROUTE_MAPPING;
    } else {
        elog(ERROR, "unknown table name '%s'", table_name);
    }

    // AttrNumber attnum;
    // foreach (lc, attrs_used_list) {
    //     attnum = lfirst_int(lc);
    // }

    cow_seFdwScanState *scan_state = (cow_seFdwScanState *) palloc0(sizeof(cow_seFdwScanState));
    node->fdw_state = scan_state;

    scan_state->deserializer = mapping.deserializer;
    // flatbuffer accessor functions indexed by attrnum
    AttributeAccessor *indexed_accessors;

    TupleTableSlot *slot = node->ss.ss_ScanTupleSlot;
    TupleDesc tupleDesc = slot->tts_tupleDescriptor;

    assert(tupleDesc->natts == mapping.num_attrs);
    indexed_accessors = (AttributeAccessor *) palloc0(sizeof(AttributeAccessor) * tupleDesc->natts);
    scan_state->indexed_accessors = indexed_accessors;

    // set up mapping of attnos to flatbuffer accessor functions
    for (int i = 0; i < tupleDesc->natts; i++) {
        // user attributes are indexed starting from 1
        // AttrNumber attnum = i + 1;
        char *attr_name = NameStr(TupleDescAttr(tupleDesc, i)->attname);
        for (int j = 0; i < mapping.num_attrs; j++) {
            if (strcmp(attr_name, mapping.attrs_with_accessors[j].attr_name) == 0) {
                indexed_accessors[i] = mapping.attrs_with_accessors[j].attr_accessor;
                break;
            }
        }
    }

    // retrieve the first node of the requested type
    scan_state->cur_node = gaia_ptr<gaia_se_node>::find_first(mapping.gaia_type_id);
}

extern "C"
TupleTableSlot *
cow_seIterateForeignScan(ForeignScanState *node)
{
    /*
     * Fetch one row from the foreign source, returning it in a tuple table
     * slot (the node's ScanTupleSlot should be used for this purpose). Return
     * NULL if no more rows are available. The tuple table slot infrastructure
     * allows either a physical or virtual tuple to be returned; in most cases
     * the latter choice is preferable from a performance standpoint. Note
     * that this is called in a short-lived memory context that will be reset
     * between invocations. Create a memory context in BeginForeignScan if you
     * need longer-lived storage, or use the es_query_cxt of the node's
     * EState.
     *
     * The rows returned must match the column signature of the foreign table
     * being scanned. If you choose to optimize away fetching columns that are
     * not needed, you should insert nulls in those column positions.
     *
     * Note that PostgreSQL's executor doesn't care whether the rows returned
     * violate any NOT NULL constraints that were defined on the foreign table
     * columns — but the planner does care, and may optimize queries
     * incorrectly if NULL values are present in a column declared not to
     * contain them. If a NULL value is encountered when the user has declared
     * that none should be present, it may be appropriate to raise an error
     * (just as you would need to do in the case of a data type mismatch).
     */

    cow_seFdwScanState *scan_state = (cow_seFdwScanState *) node->fdw_state;
    TupleTableSlot *slot = node->ss.ss_ScanTupleSlot;

    elog(DEBUG1, "entering function %s", __func__);

    // return NULL if we reach the end of iteration
    if (!scan_state->cur_node) {
        return NULL;
    }

    /* mark the slot empty */
    ExecClearTuple(slot);

    /* get the next record, if any, and fill in the slot */
    const void *obj_buf = scan_state->cur_node->payload;
    const void *obj_root = scan_state->deserializer(obj_buf);
    for (int attr_idx = 0; attr_idx < slot->tts_tupleDescriptor->natts; attr_idx++) {
        AttributeAccessor accessor = scan_state->indexed_accessors[attr_idx];
        Datum attr_val = accessor(obj_root);
        slot->tts_values[attr_idx] = attr_val;
        slot->tts_isnull[attr_idx] = false;
    }

    /* mark the slot as containing a virtual tuple */
    ExecStoreVirtualTuple(slot);

    /* now advance the current node to the next node in the iteration */
    scan_state->cur_node = scan_state->cur_node.find_next();

    /* return the slot */
    return slot;
}

extern "C"
void
cow_seReScanForeignScan(ForeignScanState *node)
{
    /*
     * Restart the scan from the beginning. Note that any parameters the scan
     * depends on may have changed value, so the new scan does not necessarily
     * return exactly the same rows.
     */

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
void
cow_seEndForeignScan(ForeignScanState *node)
{
    /*
     * End the scan and release resources. It is normally not important to
     * release palloc'd memory, but for example open files and connections to
     * remote servers should be cleaned up.
     */

    cow_seFdwScanState *scan_state = (cow_seFdwScanState *) node->fdw_state;
    // we should have reached the end of iteration
    assert(!scan_state->cur_node);

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
void
cow_seAddForeignUpdateTargets(Query *parsetree,
                                 RangeTblEntry *target_rte,
                                 Relation target_relation)
{
    /*
     * UPDATE and DELETE operations are performed against rows previously
     * fetched by the table-scanning functions. The FDW may need extra
     * information, such as a row ID or the values of primary-key columns, to
     * ensure that it can identify the exact row to update or delete. To
     * support that, this function can add extra hidden, or "junk", target
     * columns to the list of columns that are to be retrieved from the
     * foreign table during an UPDATE or DELETE.
     *
     * To do that, add TargetEntry items to parsetree->targetList, containing
     * expressions for the extra values to be fetched. Each such entry must be
     * marked resjunk = true, and must have a distinct resname that will
     * identify it at execution time. Avoid using names matching ctidN or
     * wholerowN, as the core system can generate junk columns of these names.
     *
     * This function is called in the rewriter, not the planner, so the
     * information available is a bit different from that available to the
     * planning routines. parsetree is the parse tree for the UPDATE or DELETE
     * command, while target_rte and target_relation describe the target
     * foreign table.
     *
     * If the AddForeignUpdateTargets pointer is set to NULL, no extra target
     * expressions are added. (This will make it impossible to implement
     * DELETE operations, though UPDATE may still be feasible if the FDW
     * relies on an unchanging primary key to identify rows.)
     */

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
List *
cow_sePlanForeignModify(PlannerInfo *root,
                           ModifyTable *plan,
                           Index resultRelation,
                           int subplan_index)
{
    /*
     * Perform any additional planning actions needed for an insert, update,
     * or delete on a foreign table. This function generates the FDW-private
     * information that will be attached to the ModifyTable plan node that
     * performs the update action. This private information must have the form
     * of a List, and will be delivered to BeginForeignModify during the
     * execution stage.
     *
     * root is the planner's global information about the query. plan is the
     * ModifyTable plan node, which is complete except for the fdwPrivLists
     * field. resultRelation identifies the target foreign table by its
     * rangetable index. subplan_index identifies which target of the
     * ModifyTable plan node this is, counting from zero; use this if you want
     * to index into plan->plans or other substructure of the plan node.
     *
     * If the PlanForeignModify pointer is set to NULL, no additional
     * plan-time actions are taken, and the fdw_private list delivered to
     * BeginForeignModify will be NIL.
     */

    elog(DEBUG1, "entering function %s", __func__);

    return NULL;
}

extern "C"
void
cow_seBeginForeignModify(ModifyTableState *mtstate,
                            ResultRelInfo *rinfo,
                            List *fdw_private,
                            int subplan_index,
                            int eflags)
{
    /*
     * Begin executing a foreign table modification operation. This routine is
     * called during executor startup. It should perform any initialization
     * needed prior to the actual table modifications. Subsequently,
     * ExecForeignInsert, ExecForeignUpdate or ExecForeignDelete will be
     * called for each tuple to be inserted, updated, or deleted.
     *
     * mtstate is the overall state of the ModifyTable plan node being
     * executed; global data about the plan and execution state is available
     * via this structure. rinfo is the ResultRelInfo struct describing the
     * target foreign table. (The ri_FdwState field of ResultRelInfo is
     * available for the FDW to store any private state it needs for this
     * operation.) fdw_private contains the private data generated by
     * PlanForeignModify, if any. subplan_index identifies which target of the
     * ModifyTable plan node this is. eflags contains flag bits describing the
     * executor's operating mode for this plan node.
     *
     * Note that when (eflags & EXEC_FLAG_EXPLAIN_ONLY) is true, this function
     * should not perform any externally-visible actions; it should only do
     * the minimum required to make the node state valid for
     * ExplainForeignModify and EndForeignModify.
     *
     * If the BeginForeignModify pointer is set to NULL, no action is taken
     * during executor startup.
     */

    cow_seFdwModifyState *modify_state = (cow_seFdwModifyState *) palloc0(sizeof(cow_seFdwModifyState));
    rinfo->ri_FdwState = modify_state;

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
TupleTableSlot *
cow_seExecForeignInsert(EState *estate,
                           ResultRelInfo *rinfo,
                           TupleTableSlot *slot,
                           TupleTableSlot *planSlot)
{
    /*
     * Insert one tuple into the foreign table. estate is global execution
     * state for the query. rinfo is the ResultRelInfo struct describing the
     * target foreign table. slot contains the tuple to be inserted; it will
     * match the rowtype definition of the foreign table. planSlot contains
     * the tuple that was generated by the ModifyTable plan node's subplan; it
     * differs from slot in possibly containing additional "junk" columns.
     * (The planSlot is typically of little interest for INSERT cases, but is
     * provided for completeness.)
     *
     * The return value is either a slot containing the data that was actually
     * inserted (this might differ from the data supplied, for example as a
     * result of trigger actions), or NULL if no row was actually inserted
     * (again, typically as a result of triggers). The passed-in slot can be
     * re-used for this purpose.
     *
     * The data in the returned slot is used only if the INSERT query has a
     * RETURNING clause. Hence, the FDW could choose to optimize away
     * returning some or all columns depending on the contents of the
     * RETURNING clause. However, some slot must be returned to indicate
     * success, or the query's reported rowcount will be wrong.
     *
     * If the ExecForeignInsert pointer is set to NULL, attempts to insert
     * into the foreign table will fail with an error message.
     *
     */

    /* ----
     * cow_seFdwModifyState *modify_state =
     *   (cow_seFdwModifyState *) rinfo->ri_FdwState;
     * ----
     */

    elog(DEBUG1, "entering function %s", __func__);

    return slot;
}

extern "C"
TupleTableSlot *
cow_seExecForeignUpdate(EState *estate,
                           ResultRelInfo *rinfo,
                           TupleTableSlot *slot,
                           TupleTableSlot *planSlot)
{
    /*
     * Update one tuple in the foreign table. estate is global execution state
     * for the query. rinfo is the ResultRelInfo struct describing the target
     * foreign table. slot contains the new data for the tuple; it will match
     * the rowtype definition of the foreign table. planSlot contains the
     * tuple that was generated by the ModifyTable plan node's subplan; it
     * differs from slot in possibly containing additional "junk" columns. In
     * particular, any junk columns that were requested by
     * AddForeignUpdateTargets will be available from this slot.
     *
     * The return value is either a slot containing the row as it was actually
     * updated (this might differ from the data supplied, for example as a
     * result of trigger actions), or NULL if no row was actually updated
     * (again, typically as a result of triggers). The passed-in slot can be
     * re-used for this purpose.
     *
     * The data in the returned slot is used only if the UPDATE query has a
     * RETURNING clause. Hence, the FDW could choose to optimize away
     * returning some or all columns depending on the contents of the
     * RETURNING clause. However, some slot must be returned to indicate
     * success, or the query's reported rowcount will be wrong.
     *
     * If the ExecForeignUpdate pointer is set to NULL, attempts to update the
     * foreign table will fail with an error message.
     *
     */

    /* ----
     * cow_seFdwModifyState *modify_state =
     *   (cow_seFdwModifyState *) rinfo->ri_FdwState;
     * ----
     */

    elog(DEBUG1, "entering function %s", __func__);

    return slot;
}

extern "C"
TupleTableSlot *
cow_seExecForeignDelete(EState *estate,
                           ResultRelInfo *rinfo,
                           TupleTableSlot *slot,
                           TupleTableSlot *planSlot)
{
    /*
     * Delete one tuple from the foreign table. estate is global execution
     * state for the query. rinfo is the ResultRelInfo struct describing the
     * target foreign table. slot contains nothing useful upon call, but can
     * be used to hold the returned tuple. planSlot contains the tuple that
     * was generated by the ModifyTable plan node's subplan; in particular, it
     * will carry any junk columns that were requested by
     * AddForeignUpdateTargets. The junk column(s) must be used to identify
     * the tuple to be deleted.
     *
     * The return value is either a slot containing the row that was deleted,
     * or NULL if no row was deleted (typically as a result of triggers). The
     * passed-in slot can be used to hold the tuple to be returned.
     *
     * The data in the returned slot is used only if the DELETE query has a
     * RETURNING clause. Hence, the FDW could choose to optimize away
     * returning some or all columns depending on the contents of the
     * RETURNING clause. However, some slot must be returned to indicate
     * success, or the query's reported rowcount will be wrong.
     *
     * If the ExecForeignDelete pointer is set to NULL, attempts to delete
     * from the foreign table will fail with an error message.
     */

    /* ----
     * cow_seFdwModifyState *modify_state =
     *   (cow_seFdwModifyState *) rinfo->ri_FdwState;
     * ----
     */

    elog(DEBUG1, "entering function %s", __func__);

    return slot;
}

extern "C"
void
cow_seEndForeignModify(EState *estate,
                          ResultRelInfo *rinfo)
{
    /*
     * End the table update and release resources. It is normally not
     * important to release palloc'd memory, but for example open files and
     * connections to remote servers should be cleaned up.
     *
     * If the EndForeignModify pointer is set to NULL, no action is taken
     * during executor shutdown.
     */

    /* ----
     * cow_seFdwModifyState *modify_state =
     *   (cow_seFdwModifyState *) rinfo->ri_FdwState;
     * ----
     */

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
void
cow_seBeginForeignInsert(ModifyTableState *mtstate,
                                       ResultRelInfo *resultRelInfo)
{
    elog(DEBUG1, "entering function %s", __func__);
}

extern "C"
void
cow_seEndForeignInsert(EState *estate,
                                     ResultRelInfo *resultRelInfo)
{
    elog(DEBUG1, "entering function %s", __func__);
}

extern "C"
int
cow_seIsForeignRelUpdatable(Relation rel)
{
    /*
     * Report which update operations the specified foreign table supports.
     * The return value should be a bit mask of rule event numbers indicating
     * which operations are supported by the foreign table, using the CmdType
     * enumeration; that is, (1 << CMD_UPDATE) = 4 for UPDATE, (1 <<
     * CMD_INSERT) = 8 for INSERT, and (1 << CMD_DELETE) = 16 for DELETE.
     *
     * If the IsForeignRelUpdatable pointer is set to NULL, foreign tables are
     * assumed to be insertable, updatable, or deletable if the FDW provides
     * ExecForeignInsert, ExecForeignUpdate, or ExecForeignDelete
     * respectively. This function is only needed if the FDW supports some
     * tables that are updatable and some that are not. (Even then, it's
     * permissible to throw an error in the execution routine instead of
     * checking in this function. However, this function is used to determine
     * updatability for display in the information_schema views.)
     */

    elog(DEBUG1, "entering function %s", __func__);

    return (1 << CMD_UPDATE) | (1 << CMD_INSERT) | (1 << CMD_DELETE);
}

extern "C"
bool
cow_sePlanDirectModify(PlannerInfo *root,
                                     ModifyTable *plan,
                                     Index resultRelation,
                                     int subplan_index)
{
    elog(DEBUG1, "entering function %s", __func__);

    return false;
}

extern "C"
void
cow_seBeginDirectModify(ForeignScanState *node, int eflags)
{
    elog(DEBUG1, "entering function %s", __func__);
}

extern "C"
TupleTableSlot *
cow_seIterateDirectModify(ForeignScanState *node)
{
    elog(DEBUG1, "entering function %s", __func__);
    return NULL;
}

extern "C"
void
cow_seEndDirectModify(ForeignScanState *node)
{
    elog(DEBUG1, "entering function %s", __func__);
}

extern "C"
void
cow_seExplainForeignScan(ForeignScanState *node,
                            struct ExplainState *es)
{
    /*
     * Print additional EXPLAIN output for a foreign table scan. This function
     * can call ExplainPropertyText and related functions to add fields to the
     * EXPLAIN output. The flag fields in es can be used to determine what to
     * print, and the state of the ForeignScanState node can be inspected to
     * provide run-time statistics in the EXPLAIN ANALYZE case.
     *
     * If the ExplainForeignScan pointer is set to NULL, no additional
     * information is printed during EXPLAIN.
     */

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
void
cow_seExplainForeignModify(ModifyTableState *mtstate,
                              ResultRelInfo *rinfo,
                              List *fdw_private,
                              int subplan_index,
                              struct ExplainState *es)
{
    /*
     * Print additional EXPLAIN output for a foreign table update. This
     * function can call ExplainPropertyText and related functions to add
     * fields to the EXPLAIN output. The flag fields in es can be used to
     * determine what to print, and the state of the ModifyTableState node can
     * be inspected to provide run-time statistics in the EXPLAIN ANALYZE
     * case. The first four arguments are the same as for BeginForeignModify.
     *
     * If the ExplainForeignModify pointer is set to NULL, no additional
     * information is printed during EXPLAIN.
     */

    /* ----
     * cow_seFdwModifyState *modify_state =
     *   (cow_seFdwModifyState *) rinfo->ri_FdwState;
     * ----
     */

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
void
cow_seExplainDirectModify(ForeignScanState *node,
                                        struct ExplainState *es)
{
    elog(DEBUG1, "entering function %s", __func__);
}

extern "C"
bool
cow_seAnalyzeForeignTable(Relation relation,
                             AcquireSampleRowsFunc *func,
                             BlockNumber *totalpages)
{
    /* ----
     * This function is called when ANALYZE is executed on a foreign table. If
     * the FDW can collect statistics for this foreign table, it should return
     * true, and provide a pointer to a function that will collect sample rows
     * from the table in func, plus the estimated size of the table in pages
     * in totalpages. Otherwise, return false.
     *
     * If the FDW does not support collecting statistics for any tables, the
     * AnalyzeForeignTable pointer can be set to NULL.
     *
     * If provided, the sample collection function must have the signature:
     *
     *    int
     *    AcquireSampleRowsFunc (Relation relation, int elevel,
     *                           HeapTuple *rows, int targrows,
     *                           double *totalrows,
     *                           double *totaldeadrows);
     *
     * A random sample of up to targrows rows should be collected from the
     * table and stored into the caller-provided rows array. The actual number
     * of rows collected must be returned. In addition, store estimates of the
     * total numbers of live and dead rows in the table into the output
     * parameters totalrows and totaldeadrows. (Set totaldeadrows to zero if
     * the FDW does not have any concept of dead rows.)
     * ----
     */

    elog(DEBUG1, "entering function %s", __func__);

    return false;
}

extern "C"
void
cow_seGetForeignJoinPaths(PlannerInfo *root,
                             RelOptInfo *joinrel,
                             RelOptInfo *outerrel,
                             RelOptInfo *innerrel,
                             JoinType jointype,
                             JoinPathExtraData *extra)
{
    /*
     * Create possible access paths for a join of two (or more) foreign tables
     * that all belong to the same foreign server. This optional function is
     * called during query planning. As with GetForeignPaths, this function
     * should generate ForeignPath path(s) for the supplied joinrel, and call
     * add_path to add these paths to the set of paths considered for the
     * join. But unlike GetForeignPaths, it is not necessary that this
     * function succeed in creating at least one path, since paths involving
     * local joining are always possible.
     *
     * Note that this function will be invoked repeatedly for the same join
     * relation, with different combinations of inner and outer relations; it
     * is the responsibility of the FDW to minimize duplicated work.
     *
     * If a ForeignPath path is chosen for the join, it will represent the
     * entire join process; paths generated for the component tables and
     * subsidiary joins will not be used. Subsequent processing of the join
     * path proceeds much as it does for a path scanning a single foreign
     * table. One difference is that the scanrelid of the resulting
     * ForeignScan plan node should be set to zero, since there is no single
     * relation that it represents; instead, the fs_relids field of the
     * ForeignScan node represents the set of relations that were joined. (The
     * latter field is set up automatically by the core planner code, and need
     * not be filled by the FDW.) Another difference is that, because the
     * column list for a remote join cannot be found from the system catalogs,
     * the FDW must fill fdw_scan_tlist with an appropriate list of
     * TargetEntry nodes, representing the set of columns it will supply at
     * runtime in the tuples it returns.
     */

    elog(DEBUG1, "entering function %s", __func__);

}

extern "C"
void
cow_seGetForeignUpperPaths(PlannerInfo *root,
                             UpperRelationKind stage,
                             RelOptInfo *input_rel,
                             RelOptInfo *output_rel,
                             void *extra)
{
    elog(DEBUG1, "entering function %s", __func__);
}

extern "C"
bool
cow_seRecheckForeignScan(ForeignScanState *node,
                                       TupleTableSlot *slot)
{
    elog(DEBUG1, "entering function %s", __func__);

    return false;
}

extern "C"
RowMarkType
cow_seGetForeignRowMarkType(RangeTblEntry *rte,
                               LockClauseStrength strength)
{
    /*
     * Report which row-marking option to use for a foreign table. rte is the
     * RangeTblEntry node for the table and strength describes the lock
     * strength requested by the relevant FOR UPDATE/SHARE clause, if any. The
     * result must be a member of the RowMarkType enum type.
     *
     * This function is called during query planning for each foreign table
     * that appears in an UPDATE, DELETE, or SELECT FOR UPDATE/SHARE query and
     * is not the target of UPDATE or DELETE.
     *
     * If the GetForeignRowMarkType pointer is set to NULL, the ROW_MARK_COPY
     * option is always used. (This implies that RefetchForeignRow will never
     * be called, so it need not be provided either.)
     */

    elog(DEBUG1, "entering function %s", __func__);

    return ROW_MARK_COPY;

}

extern "C"
void
cow_seRefetchForeignRow(EState *estate,
                          ExecRowMark *erm,
                          Datum rowid,
                          TupleTableSlot *slot,
                          bool *updated)
{
    /*
     * Re-fetch one tuple from the foreign table, after locking it if
     * required. estate is global execution state for the query. erm is the
     * ExecRowMark struct describing the target foreign table and the row lock
     * type (if any) to acquire. rowid identifies the tuple to be fetched.
     * updated is an output parameter.
     *
     * This function should return a palloc'ed copy of the fetched tuple, or
     * NULL if the row lock couldn't be obtained. The row lock type to acquire
     * is defined by erm->markType, which is the value previously returned by
     * GetForeignRowMarkType. (ROW_MARK_REFERENCE means to just re-fetch the
     * tuple without acquiring any lock, and ROW_MARK_COPY will never be seen
     * by this routine.)
     *
     * In addition, *updated should be set to true if what was fetched was an
     * updated version of the tuple rather than the same version previously
     * obtained. (If the FDW cannot be sure about this, always returning true
     * is recommended.)
     *
     * Note that by default, failure to acquire a row lock should result in
     * raising an error; a NULL return is only appropriate if the SKIP LOCKED
     * option is specified by erm->waitPolicy.
     *
     * The rowid is the ctid value previously read for the row to be
     * re-fetched. Although the rowid value is passed as a Datum, it can
     * currently only be a tid. The function API is chosen in hopes that it
     * may be possible to allow other datatypes for row IDs in future.
     *
     * If the RefetchForeignRow pointer is set to NULL, attempts to re-fetch
     * rows will fail with an error message.
     */

    elog(DEBUG1, "entering function %s", __func__);
}


// we may want to use IMPORT FOREIGN SCHEMA to automatically create the foreign
// tables corresponding to registered types in the storage engine.
extern "C"
List *
cow_seImportForeignSchema(ImportForeignSchemaStmt *stmt,
                             Oid serverOid)
{
    /*
     * Obtain a list of foreign table creation commands. This function is
     * called when executing IMPORT FOREIGN SCHEMA, and is passed the parse
     * tree for that statement, as well as the OID of the foreign server to
     * use. It should return a list of C strings, each of which must contain a
     * CREATE FOREIGN TABLE command. These strings will be parsed and executed
     * by the core server.
     *
     * Within the ImportForeignSchemaStmt struct, remote_schema is the name of
     * the remote schema from which tables are to be imported. list_type
     * identifies how to filter table names: FDW_IMPORT_SCHEMA_ALL means that
     * all tables in the remote schema should be imported (in this case
     * table_list is empty), FDW_IMPORT_SCHEMA_LIMIT_TO means to include only
     * tables listed in table_list, and FDW_IMPORT_SCHEMA_EXCEPT means to
     * exclude the tables listed in table_list. options is a list of options
     * used for the import process. The meanings of the options are up to the
     * FDW. For example, an FDW could use an option to define whether the NOT
     * NULL attributes of columns should be imported. These options need not
     * have anything to do with those supported by the FDW as database object
     * options.
     *
     * The FDW may ignore the local_schema field of the
     * ImportForeignSchemaStmt, because the core server will automatically
     * insert that name into the parsed CREATE FOREIGN TABLE commands.
     *
     * The FDW does not have to concern itself with implementing the filtering
     * specified by list_type and table_list, either, as the core server will
     * automatically skip any returned commands for tables excluded according
     * to those options. However, it's often useful to avoid the work of
     * creating commands for excluded tables in the first place. The function
     * IsImportableForeignTable() may be useful to test whether a given
     * foreign-table name will pass the filter.
     */

    elog(DEBUG1, "entering function %s", __func__);

    return NULL;
}

// Perform all module-level initialization here
extern "C"
void
_PG_init() {
    // initialize COW-SE
    gaia_mem_base::init(true);
}

// Perform all module-level finalization here
extern "C"
void
_PG_fini() {
    // any SE cleanup required?
}
