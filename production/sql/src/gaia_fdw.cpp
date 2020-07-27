/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_fdw.hpp"

using namespace std;
using namespace gaia::db;

// Magic block for extension library.
extern "C" {
PG_MODULE_MAGIC;
}

// HACKHACK: global counter to simulate nested transactions. Because a DELETE
// plan is nested within a scan, committing the write txn will invalidate the
// read txn. We get around this by using a refcount to track the txn nesting
// state, so we only open a txn when the counter is initially incremented from 0
// and only commit a txn when the counter is decremented to 0. An unsynchronized
// global counter is ok since there's no concurrency within a postgres backend.
static int gaia_txn_ref_count
    // Use signed int so we can assert it is non-negative.
    = 0;

static bool is_gaia_txn_open() {
    assert(gaia_txn_ref_count >= 0);
    return gaia_txn_ref_count > 0;
}

static bool begin_gaia_txn() {
    elog(DEBUG1, "Opening COW-SE transaction...");

    bool txn_opened = false;
    assert(gaia_txn_ref_count >= 0);

    int old_count = gaia_txn_ref_count++;
    if (old_count == 0) {
        txn_opened = true;
        gaia::db::begin_transaction();
    }

    elog(DEBUG1, "Txn actually opened: %s.", txn_opened ? "true" : "false");

    return txn_opened;
}

static bool commit_gaia_txn() {
    elog(DEBUG1, "Closing COW-SE transaction...");

    bool txn_closed = false;
    assert(gaia_txn_ref_count > 0);

    int old_count = gaia_txn_ref_count--;
    if (old_count == 1) {
        txn_closed = true;
        gaia::db::commit_transaction();
    }

    elog(DEBUG1, "Txn actually closed: %s.", txn_closed ? "true" : "false");

    return txn_closed;
}

// The FDW handler function returns a palloc'd FdwRoutine struct containing
// pointers to the callback functions that will be called by the planner,
// executor, and various maintenance commands. The scan-related functions are
// required, the rest are optional.
extern "C" Datum gaia_fdw_handler(PG_FUNCTION_ARGS) {
    elog(DEBUG1, "Entering function %s...", __func__);

    FdwRoutine *routine = makeNode(FdwRoutine);

    // Functions for scanning foreign tables.
    routine->GetForeignRelSize = gaia_get_foreign_rel_size;
    routine->GetForeignPaths = gaia_get_foreign_paths;
    routine->GetForeignPlan = gaia_get_foreign_plan;
    routine->BeginForeignScan = gaia_begin_foreign_scan;
    routine->IterateForeignScan = gaia_iterate_foreign_scan;
    routine->ReScanForeignScan = gaia_rescan_foreign_scan;
    routine->EndForeignScan = gaia_end_foreign_scan;

    // Functions for updating foreign tables.
    routine->AddForeignUpdateTargets = gaia_add_foreign_update_targets;
    routine->PlanForeignModify = gaia_plan_foreign_modify;
    routine->BeginForeignModify = gaia_begin_foreign_modify;
    routine->ExecForeignInsert = gaia_exec_foreign_insert;
    routine->ExecForeignUpdate = gaia_exec_foreign_update;
    routine->ExecForeignDelete = gaia_exec_foreign_delete;
    routine->EndForeignModify = gaia_end_foreign_modify;
    routine->BeginForeignInsert = gaia_begin_foreign_insert;
    routine->EndForeignInsert = gaia_end_foreign_insert;
    routine->IsForeignRelUpdatable = gaia_is_foreign_rel_updatable;
    routine->PlanDirectModify = gaia_plan_direct_modify;
    routine->BeginDirectModify = gaia_begin_direct_modify;
    routine->IterateDirectModify = gaia_iterate_direct_modify;
    routine->EndDirectModify = gaia_end_direct_modify;

    // Function for EvalPlanQual rechecks.
    routine->RecheckForeignScan = gaia_recheck_foreign_scan;
    // Support functions for EXPLAIN.
    routine->ExplainForeignScan = gaia_explain_foreign_scan;
    routine->ExplainForeignModify = gaia_explain_foreign_modify;
    routine->ExplainDirectModify = gaia_explain_direct_modify;

    // Support functions for ANALYZE.
    routine->AnalyzeForeignTable = gaia_analyze_foreign_table;

    // Support functions for IMPORT FOREIGN SCHEMA.
    routine->ImportForeignSchema = gaia_import_foreign_schema;

    // Support functions for join push-down.
    routine->GetForeignJoinPaths = gaia_get_foreign_join_paths;

    // Support functions for upper relation push-down.
    routine->GetForeignUpperPaths = gaia_get_foreign_upper_paths;

    // Support functions for late row locking.
    routine->RecheckForeignScan = gaia_recheck_foreign_scan;
    routine->GetForeignRowMarkType = gaia_get_foreign_row_mark_type;
    routine->RefetchForeignRow = gaia_refetch_foreign_row;

    PG_RETURN_POINTER(routine);
}

// Check if the provided option is one of the valid options.
// context is the Oid of the catalog holding the object the option is for.
// If handler is registered for this option, invoke it.
static bool is_valid_option(const char *option, const char *value, Oid context) {
    const gaia_fdw_option_t *opt;
    for (opt = valid_options; opt->name; opt++) {
        if (context == opt->context && strcmp(opt->name, option) == 0) {
            // Invoke option handler callback.
            opt->handler(option, value, context);
            return true;
        }
    }
    return false;
}

// The validator function is responsible for validating options given in CREATE
// and ALTER commands for its foreign data wrapper, as well as foreign servers,
// user mappings, and foreign tables using the wrapper. The validator function
// must be registered as taking two arguments, a text array containing the
// options to be validated, and an OID representing the type of object the
// options are associated with (in the form of the OID of the system catalog
// the object would be stored in, either ForeignDataWrapperRelationId,
// ForeignServerRelationId, UserMappingRelationId, or ForeignTableRelationId).
// If no validator function is supplied, options are not checked at object
// creation time or object alteration time.
extern "C" Datum gaia_fdw_validator(PG_FUNCTION_ARGS) {
    elog(DEBUG1, "Entering function %s...", __func__);

    List *options_list = untransformRelOptions(PG_GETARG_DATUM(0));
    if (list_length(options_list) > 1) {
        ereport(ERROR,
            (errcode(ERRCODE_FDW_INVALID_OPTION_NAME),
            errmsg("Invalid options."),
            errhint("gaia FDW supports only the `data_dir` option.")));
        PG_RETURN_VOID();
    }

    Oid catalog = PG_GETARG_OID(1);
    ListCell *cell;
    foreach (cell, options_list) {
        DefElem *def = (DefElem *)lfirst(cell);
        char *opt_name = def->defname;
        char *opt_val = defGetString(def);
        elog(DEBUG1, "Option name: %s, option value: %s.", opt_name, opt_val);

        if (!is_valid_option(opt_name, opt_val, catalog)) {
            // Unknown option specified, complain about it. Provide a hint
            // with list of valid options for the object.
            const gaia_fdw_option_t *opt;
            StringInfoData buf;
            initStringInfo(&buf);

            for (opt = valid_options; opt->name; opt++) {
                if (catalog == opt->context) {
                    appendStringInfo(&buf, "%s%s", (buf.len > 0) ? ", " : "",
                        opt->name);
                }
            }

            ereport(
                ERROR,
                (errcode(ERRCODE_FDW_INVALID_OPTION_NAME),
                errmsg("Invalid option \"%s\".", opt_name),
                buf.len > 0
                    ? errhint("Valid options in this context are: %s.", buf.data)
                    : errhint("There are no valid options in this context.")));

            PG_RETURN_VOID();
        }
    }

    PG_RETURN_VOID();
}

extern "C" void gaia_get_foreign_rel_size(
    PlannerInfo *root,
    RelOptInfo *base_rel,
    Oid foreign_table_id) {
    // Obtain relation size estimates for a foreign table. This is called at
    // the beginning of planning for a query that scans a foreign table. root
    // is the planner's global information about the query; base_rel is the
    // planner's information about this table; and foreign_table_id is the
    // pg_class OID of the foreign table. (foreign_table_id could be obtained
    // from the planner data structures, but it's passed explicitly to save
    // effort.)
    //
    // This function should update base_rel->rows to be the expected number of
    // rows returned by the table scan, after accounting for the filtering
    // done by the restriction quals. The initial value of base_rel->rows is
    // just a constant default estimate, which should be replaced if at all
    // possible. The function may also choose to update base_rel->width if it
    // can compute a better estimate of the average result row width.
    elog(DEBUG1, "Entering function %s...", __func__);

    // TODO: get row count estimate from storage engine.
}

extern "C" void gaia_get_foreign_paths(
    PlannerInfo *root,
    RelOptInfo *base_rel,
    Oid foreign_table_id) {
    // Create possible access paths for a scan on a foreign table. This is
    // called during query planning. The parameters are the same as for
    // GetForeignRelSize, which has already been called.
    //
    // This function must generate at least one access path (ForeignPath node)
    // for a scan on the foreign table and must call add_path to add each such
    // path to base_rel->pathlist. It's recommended to use
    // create_foreignscan_path to build the ForeignPath nodes. The function
    // can generate multiple access paths, e.g., a path which has valid
    // pathkeys to represent a pre-sorted result. Each access path must
    // contain cost estimates, and can contain any FDW-private information
    // that is needed to identify the specific scan method intended.
    elog(DEBUG1, "Entering function %s...", __func__);

    Cost startup_cost = 0;
    Cost total_cost = startup_cost + base_rel->rows;

    Path* foreign_scan_path = (Path *)create_foreignscan_path(
            root,
            base_rel,
            // Default pathtarget.
            NULL,
            base_rel->rows,
            startup_cost,
            total_cost,
            // No pathkeys.
            NIL,
            // No outer rel either.
            NULL,
            // No extra plan.
            NULL,
            // No per-path private info.
            NULL);

    // Create a ForeignPath node and add it as only possible path.
    add_path(base_rel, foreign_scan_path);
}

extern "C" ForeignScan *gaia_get_foreign_plan(
    PlannerInfo *root,
    RelOptInfo *base_rel,
    Oid foreign_table_id,
    ForeignPath *best_path,
    List *tlist,
    List *scan_clauses,
    Plan *outer_plan) {
    // Create a ForeignScan plan node from the selected foreign access path.
    // This is called at the end of query planning. The parameters are as for
    // GetForeignRelSize, plus the selected ForeignPath (previously produced
    // by GetForeignPaths), the target list to be emitted by the plan node,
    // and the restriction clauses to be enforced by the plan node.
    //
    // This function must create and return a ForeignScan plan node; it's
    // recommended to use make_foreignscan to build the ForeignScan node.
    elog(DEBUG1, "Entering function %s...", __func__);

    Index scan_relid = base_rel->relid;

    // We have no native ability to evaluate restriction clauses, so we just
    // put all the scan_clauses into the plan node's qual list for the
    // executor to check. So all we have to do here is strip RestrictInfo
    // nodes from the clauses and ignore pseudoconstants (which will be
    // handled elsewhere).
    scan_clauses = extract_actual_clauses(scan_clauses, false);

    // Create the ForeignScan node.
    return make_foreignscan(tlist, scan_clauses, scan_relid,
        // No expressions to evaluate.
        NIL,
        // No private data.
        NIL,
        // No custom tlist.
        NIL,
        // No remote quals.
        NIL,
        outer_plan);
}

extern "C" void gaia_begin_foreign_scan(ForeignScanState *node, int eflags) {
    // Begin executing a foreign scan. This is called during executor startup.
    // It should perform any initialization needed before the scan can start,
    // but not start executing the actual scan (that should be done upon the
    // first call to IterateForeignScan). The ForeignScanState node has
    // already been created, but its fdw_state field is still NULL.
    // Information about the table to scan is accessible through the
    // ForeignScanState node (in particular, from the underlying ForeignScan
    // plan node, which contains any FDW-private information provided by
    // GetForeignPlan). eflags contains flag bits describing the executor's
    // operating mode for this plan node.
    //
    // Note that when (eflags & EXEC_FLAG_EXPLAIN_ONLY) is true, this function
    // should not perform any externally-visible actions; it should only do
    // the minimum required to make the node state valid for
    // ExplainForeignScan and EndForeignScan.
    elog(DEBUG1, "Entering function %s...", __func__);

    ForeignScan *plan = (ForeignScan *)node->ss.ps.plan;
    EState *estate = node->ss.ps.state;
    Index rtindex = plan->scan.scanrelid;
    RangeTblEntry *rte = exec_rt_fetch(rtindex, estate);
    char *table_name = get_rel_name(rte->relid);

    relation_attribute_mapping_t mapping;
    if (strcmp(table_name, "airports") == 0) {
        mapping = c_airport_mapping;
    } else if (strcmp(table_name, "airlines") == 0) {
        mapping = c_airline_mapping;
    } else if (strcmp(table_name, "routes") == 0) {
        mapping = c_route_mapping;
    } else if (strcmp(table_name, "event_log") == 0) {
        mapping = c_event_log_mapping;
    } else {
        elog(ERROR, "Unknown table name '%s'.", table_name);
    }

    TupleTableSlot *slot = node->ss.ss_ScanTupleSlot;
    TupleDesc tupleDesc = slot->tts_tupleDescriptor;

    assert((size_t)tupleDesc->natts == mapping.attribute_count);

    // flatbuffer accessor functions indexed by attrnum.
    attribute_accessor_fn *indexed_accessors = (attribute_accessor_fn *)palloc0(
        sizeof(attribute_accessor_fn) * tupleDesc->natts);

    // Set up mapping of attnos to flatbuffer accessor functions.
    for (int i = 0; i < tupleDesc->natts; i++) {
        // User attributes are indexed starting from 1.
        // AttrNumber attnum = i + 1.
        char *attr_name = NameStr(TupleDescAttr(tupleDesc, i)->attname);
        for (size_t j = 0; j < mapping.attribute_count; j++) {
            if (strcmp(attr_name, mapping.attributes[j].name) == 0) {
                indexed_accessors[i] = mapping.attributes[j].accessor;
                break;
            }
        }
    }

    gaia_fdw_scan_state_t *scan_state
        = (gaia_fdw_scan_state_t *)palloc0(sizeof(gaia_fdw_scan_state_t));

    scan_state->indexed_accessors = indexed_accessors;
    scan_state->deserializer = mapping.deserializer;

    node->fdw_state = scan_state;

    // Begin read transaction.
    begin_gaia_txn();

    // Retrieve the first node of the requested type
    // (this can't currently throw).
    scan_state->cur_node = gaia_ptr::find_first(mapping.gaia_type_id);
}

extern "C" TupleTableSlot *gaia_iterate_foreign_scan(ForeignScanState *node) {
    // Fetch one row from the foreign source, returning it in a tuple table
    // slot (the node's ScanTupleSlot should be used for this purpose). Return
    // NULL if no more rows are available. The tuple table slot infrastructure
    // allows either a physical or virtual tuple to be returned; in most cases
    // the latter choice is preferable from a performance standpoint. Note
    // that this is called in a short-lived memory context that will be reset
    // between invocations. Create a memory context in BeginForeignScan if you
    // need longer-lived storage, or use the es_query_cxt of the node's
    // EState.
    //
    // The rows returned must match the column signature of the foreign table
    // being scanned. If you choose to optimize away fetching columns that are
    // not needed, you should insert nulls in those column positions.
    //
    // Note that PostgreSQL's executor doesn't care whether the rows returned
    // violate any NOT NULL constraints that were defined on the foreign table
    // columns — but the planner does care, and may optimize queries
    // incorrectly if NULL values are present in a column declared not to
    // contain them. If a NULL value is encountered when the user has declared
    // that none should be present, it may be appropriate to raise an error
    // (just as you would need to do in the case of a data type mismatch).
    // elog(DEBUG1, "Entering function %s...", __func__);

    gaia_fdw_scan_state_t *scan_state = (gaia_fdw_scan_state_t *)node->fdw_state;
    TupleTableSlot *slot = node->ss.ss_ScanTupleSlot;

    // Return NULL if we reach the end of iteration.
    if (!scan_state->cur_node) {
        return NULL;
    }

    // Mark the slot empty.
    ExecClearTuple(slot);

    // Get the next record, if any, and fill in the slot.
    const void *obj_buf;
    obj_buf = scan_state->cur_node.data();
    const void *obj_root = scan_state->deserializer(obj_buf);

    for (int attr_idx = 0; attr_idx < slot->tts_tupleDescriptor->natts; attr_idx++) {
        char *attr_name = NameStr(
            TupleDescAttr(slot->tts_tupleDescriptor, attr_idx)->attname);

        attribute_accessor_fn accessor = scan_state->indexed_accessors[attr_idx];
        Datum attr_val = accessor(obj_root);

        slot->tts_values[attr_idx] = attr_val;
        slot->tts_isnull[attr_idx] = false;
    }

    // Mark the slot as containing a virtual tuple.
    ExecStoreVirtualTuple(slot);

    // Now advance the current node to the next node in the iteration
    // (this can't currently throw).
    scan_state->cur_node = scan_state->cur_node.find_next();

    // Return the slot.
    return slot;
}

extern "C" void gaia_rescan_foreign_scan(ForeignScanState *node) {
    // Restart the scan from the beginning. Note that any parameters the scan
    // depends on may have changed value, so the new scan does not necessarily
    // return exactly the same rows.
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" void gaia_end_foreign_scan(ForeignScanState *node) {
    // End the scan and release resources. It is normally not important to
    // release palloc'd memory, but for example open files and connections to
    // remote servers should be cleaned up.
    elog(DEBUG1, "Entering function %s...", __func__);

    gaia_fdw_scan_state_t *scan_state = (gaia_fdw_scan_state_t *)node->fdw_state;

    // We should have reached the end of iteration.
    assert(!scan_state->cur_node);

    // Commit read transaction.
    commit_gaia_txn();
}

extern "C" void gaia_add_foreign_update_targets(
    Query *parse_tree,
    RangeTblEntry *target_rte,
    Relation target_relation) {
    // UPDATE and DELETE operations are performed against rows previously
    // fetched by the table-scanning functions. The FDW may need extra
    // information, such as a row ID or the values of primary-key columns, to
    // ensure that it can identify the exact row to update or delete. To
    // support that, this function can add extra hidden, or "junk", target
    // columns to the list of columns that are to be retrieved from the
    // foreign table during an UPDATE or DELETE.
    //
    // To do that, add TargetEntry items to parse_tree->targetList, containing
    // expressions for the extra values to be fetched. Each such entry must be
    // marked resjunk = true, and must have a distinct resname that will
    // identify it at execution time. Avoid using names matching ctidN or
    // wholerowN, as the core system can generate junk columns of these names.
    //
    // This function is called in the rewriter, not the planner, so the
    // information available is a bit different from that available to the
    // planning routines. parse_tree is the parse tree for the UPDATE or DELETE
    // command, while target_rte and target_relation describe the target
    // foreign table.
    //
    // If the AddForeignUpdateTargets pointer is set to NULL, no extra target
    // expressions are added. (This will make it impossible to implement
    // DELETE operations, though UPDATE may still be feasible if the FDW
    // relies on an unchanging primary key to identify rows.)
    elog(DEBUG1, "Entering function %s...", __func__);

    TupleDesc tupleDesc = target_relation->rd_att;

    // FIXME: we really shouldn't hardcode the primary key like this,
    // instead check the attribute-level CREATE FOREIGN TABLE options
    // for KEY=true (which should be set in IMPORT FOREIGN SCHEMA).
    //
    // Loop through all columns of the foreign table.
    bool key_found = false;
    for (int i = 0; i < tupleDesc->natts; i++) {
        Form_pg_attribute attr = TupleDescAttr(tupleDesc, i);
        char *attr_name = NameStr(attr->attname);

        // If primary key, add a resjunk for this column.
        if (strcmp("gaia_id", attr_name) == 0) {
            key_found = true;

            // Make a Var representing the desired value.
            Var *var = makeVar(
                parse_tree->resultRelation,
                attr->attnum,
                attr->atttypid,
                attr->atttypmod,
                attr->attcollation,
                0);

            // Wrap it in a resjunk TLE with the right name.
            TargetEntry *tle = makeTargetEntry(
                (Expr *)var, list_length(parse_tree->targetList) + 1,
                pstrdup(NameStr(attr->attname)), true);

            // And add it to the query's targetlist.
            parse_tree->targetList = lappend(parse_tree->targetList, tle);
            break;
        }
    }

    if (!key_found) {
        elog(ERROR, "Could not find 'gaia_id' column in table '%s'.",
            RelationGetRelationName(target_relation));
    }
}

extern "C" List *gaia_plan_foreign_modify(
    PlannerInfo *root,
    ModifyTable *plan,
    Index result_relation,
    int subplan_index) {
    // Perform any additional planning actions needed for an insert, update,
    // or delete on a foreign table. This function generates the FDW-private
    // information that will be attached to the ModifyTable plan node that
    // performs the update action. This private information must have the form
    // of a List, and will be delivered to BeginForeignModify during the
    // execution stage.
    //
    // root is the planner's global information about the query. plan is the
    // ModifyTable plan node, which is complete except for the fdwPrivLists
    // field. result_relation identifies the target foreign table by its
    // rangetable index. subplan_index identifies which target of the
    // ModifyTable plan node this is, counting from zero; use this if you want
    // to index into plan->plans or other substructure of the plan node.
    //
    // If the PlanForeignModify pointer is set to NULL, no additional
    // plan-time actions are taken, and the fdw_private list delivered to
    // BeginForeignModify will be NIL.
    elog(DEBUG1, "Entering function %s...", __func__);

    // We don't return any private data from this method, just check that
    // gaia_id is not an INSERT or UPDATE target.
    CmdType operation = plan->operation;
    RangeTblEntry *rte = planner_rt_fetch(result_relation, root);
    Relation rel = table_open(rte->relid, NoLock);
    TupleDesc tupleDesc = RelationGetDescr(rel);
    Bitmapset *modifiedCols = NULL;
    int col = -1;

    if (operation == CMD_INSERT) {
        modifiedCols = rte->insertedCols;
    } else if (operation == CMD_UPDATE) {
        modifiedCols = rte->updatedCols;
    }

    if (modifiedCols) {
        while ((col = bms_next_member(modifiedCols, col)) >= 0) {
            // Bit numbers are offset by FirstLowInvalidHeapAttributeNumber.
            AttrNumber attno = col + FirstLowInvalidHeapAttributeNumber;
            if (attno <= InvalidAttrNumber) {
                // Shouldn't happen.
                elog(ERROR, "System-column insert or update is not supported.");
            }

            char *attr_name = NameStr(TupleDescAttr(tupleDesc, attno - 1)->attname);
            if (strcmp(attr_name, "gaia_id") == 0) {
                ereport(ERROR,
                    (errcode(ERRCODE_FDW_INVALID_COLUMN_NAME),
                    errmsg("Cannot insert into or update system column gaia_id.")));
            }
        }
    }

    table_close(rel, NoLock);

    return NULL;
}

extern "C" void gaia_begin_foreign_modify(
    ModifyTableState *mtstate,
    ResultRelInfo *rinfo,
    List *fdw_private,
    int subplan_index,
    int eflags) {
    // Begin executing a foreign table modification operation. This routine is
    // called during executor startup. It should perform any initialization
    // needed prior to the actual table modifications. Subsequently,
    // ExecForeignInsert, ExecForeignUpdate or ExecForeignDelete will be
    // called for each tuple to be inserted, updated, or deleted.
    //
    // mtstate is the overall state of the ModifyTable plan node being
    // executed; global data about the plan and execution state is available
    // via this structure. rinfo is the ResultRelInfo struct describing the
    // target foreign table. (The ri_FdwState field of ResultRelInfo is
    // available for the FDW to store any private state it needs for this
    // operation.) fdw_private contains the private data generated by
    // PlanForeignModify, if any. subplan_index identifies which target of the
    // ModifyTable plan node this is. eflags contains flag bits describing the
    // executor's operating mode for this plan node.
    //
    // Note that when (eflags & EXEC_FLAG_EXPLAIN_ONLY) is true, this function
    // should not perform any externally-visible actions; it should only do
    // the minimum required to make the node state valid for
    // ExplainForeignModify and EndForeignModify.
    //
    // If the BeginForeignModify pointer is set to NULL, no action is taken
    // during executor startup.
    elog(DEBUG1, "Entering function %s...", __func__);

    TupleDesc tupleDesc = rinfo->ri_RelationDesc->rd_att;
    RangeTblEntry *rte
        = exec_rt_fetch(rinfo->ri_RangeTableIndex, mtstate->ps.state);
    char *table_name = get_rel_name(rte->relid);

    gaia_fdw_modify_state_t *modify_state
        = (gaia_fdw_modify_state_t *)palloc0(sizeof(gaia_fdw_modify_state_t));

    // Set invalid values.
    modify_state->pk_attr_idx = -1;
    modify_state->src_attr_idx = -1;
    modify_state->dst_attr_idx = -1;

    relation_attribute_mapping_t mapping;
    if (strcmp(table_name, "airports") == 0) {
        mapping = c_airport_mapping;
    } else if (strcmp(table_name, "airlines") == 0) {
        mapping = c_airline_mapping;
    } else if (strcmp(table_name, "routes") == 0) {
        mapping = c_route_mapping;
    } else if (strcmp(table_name, "event_log") == 0) {
        mapping = c_event_log_mapping;
    } else {
        elog(ERROR, "Unknown table name '%s'.", table_name);
    }

    modify_state->gaia_type_id = mapping.gaia_type_id;
    modify_state->initializer = mapping.initializer;
    modify_state->finalizer = mapping.finalizer;

    flatcc_builder_init(&modify_state->builder);

    assert((size_t)tupleDesc->natts == mapping.attribute_count);
    // flatbuffer accessor functions indexed by attrnum.
    attribute_builder_fn *indexed_builders = (attribute_builder_fn *)palloc0(
        sizeof(attribute_builder_fn) * tupleDesc->natts);

    // Set up mapping of attnos to flatbuffer attribute builder functions.
    for (int i = 0; i < tupleDesc->natts; i++) {
        // User attributes are indexed starting from 1.
        // AttrNumber attnum = i + 1.
        char *attr_name = NameStr(TupleDescAttr(tupleDesc, i)->attname);
        if (strcmp(attr_name, "gaia_id") == 0) {
            modify_state->pk_attr_idx = i;
        }

        for (size_t j = 0; j < mapping.attribute_count; j++) {
            if (strcmp(attr_name, mapping.attributes[j].name) == 0) {
                indexed_builders[i] = mapping.attributes[j].builder;
                break;
            }
        }
    }

    modify_state->indexed_builders = indexed_builders;

    rinfo->ri_FdwState = modify_state;

    // Check invariants.
    assert(modify_state->pk_attr_idx > -1);

    begin_gaia_txn();
}

// Generate random gaia_id for INSERTs using /dev/urandom.
// NB: because this is a 64-bit value, we will expect collisions
// as the number of generated keys approaches 2^32! This is just
// a temporary hack until the gaia_id type becomes a 128-bit GUID.
static uint64_t new_gaia_id() {
    ifstream urandom("/dev/urandom", ios::in | ios::binary);

    if (urandom) {
        uint64_t rand_val;
        urandom.read(reinterpret_cast<char *>(&rand_val), sizeof(rand_val));

        if (urandom) {
            // Generating 0 should be statistically impossible.
            assert(rand_val != 0);
            return rand_val;
        } else {
            elog(ERROR, "Failed to read from /dev/urandom.");
        }
    } else {
        elog(ERROR, "Failed to open /dev/urandom.");
    }

    // Should never get here (because elog(ERROR) calls longjmp).
    assert(false);

    return 0;
}

extern "C" TupleTableSlot *gaia_exec_foreign_insert(
    EState *estate,
    ResultRelInfo *rinfo,
    TupleTableSlot *slot,
    TupleTableSlot *plan_slot) {
    // Insert one tuple into the foreign table. estate is global execution
    // state for the query. rinfo is the ResultRelInfo struct describing the
    // target foreign table. slot contains the tuple to be inserted; it will
    // match the rowtype definition of the foreign table. plan_slot contains
    // the tuple that was generated by the ModifyTable plan node's subplan; it
    // differs from slot in possibly containing additional "junk" columns.
    // (The plan_slot is typically of little interest for INSERT cases, but is
    // provided for completeness.)
    //
    // The return value is either a slot containing the data that was actually
    // inserted (this might differ from the data supplied, for example as a
    // result of trigger actions), or NULL if no row was actually inserted
    // (again, typically as a result of triggers). The passed-in slot can be
    // re-used for this purpose.
    //
    // The data in the returned slot is used only if the INSERT query has a
    // RETURNING clause. Hence, the FDW could choose to optimize away
    // returning some or all columns depending on the contents of the
    // RETURNING clause. However, some slot must be returned to indicate
    // success, or the query's reported rowcount will be wrong.
    //
    // If the ExecForeignInsert pointer is set to NULL, attempts to insert
    // into the foreign table will fail with an error message.
    elog(DEBUG1, "Entering function %s...", __func__);

    gaia_fdw_modify_state_t *modify_state = (gaia_fdw_modify_state_t *)rinfo->ri_FdwState;
    flatcc_builder_t *builder = &modify_state->builder;
    modify_state->initializer(builder);

    // NB: we assume 0 is a valid sentinel value, i.e., it can never be a
    // system-generated gaia_id. This is true for our internal implementation,
    // but also needs to hold for all other implementations (until the storage
    // engine itself starts generating gaia_ids).
    uint64_t gaia_id = 0;
    uint64_t gaia_src_id = 0;
    uint64_t gaia_dst_id = 0;

    // slot_getallattrs() is necessary beginning in Postgres 12 (the slot will
    // be empty!).
    // TODO: use slot_getattr()?
    slot_getallattrs(slot);
    for (int attr_idx = 0; attr_idx < slot->tts_tupleDescriptor->natts; attr_idx++) {
        char *attr_name = NameStr(
            TupleDescAttr(slot->tts_tupleDescriptor, attr_idx)->attname);

        attribute_builder_fn attr_builder = modify_state->indexed_builders[attr_idx];

        // We don't allow gaia_id to be set by an INSERT or UPDATE statement
        // (this should have already been checked in gaia_plan_foreign_modify), and
        // the storage engine doesn't yet generate gaia_ids, so we generate a
        // random gaia_id ourselves.
        Datum attr_val;
        if (attr_idx == modify_state->pk_attr_idx) {
            assert(slot->tts_isnull[attr_idx]);
            gaia_id = new_gaia_id();
            attr_val = UInt64GetDatum(gaia_id);
            slot->tts_isnull[attr_idx] = false;
        } else if (!(slot->tts_isnull[attr_idx])) {
            attr_val = slot->tts_values[attr_idx];
        }

        // If we have a null value, just don't bother to set it in the builder.
        if (!slot->tts_isnull[attr_idx]) {
            attr_builder(builder, attr_val);
        }
    }

    assert(gaia_id);

    modify_state->finalizer(builder);

    size_t size;
    const void *buffer = flatcc_builder_get_direct_buffer(builder, &size);

    try {
        gaia_ptr::create(gaia_id, modify_state->gaia_type_id, size, buffer);
    } catch (const std::exception &e) {
        ereport(ERROR,
            (errcode(ERRCODE_FDW_ERROR),
            errmsg("Error creating gaia object."), errhint(e.what())));
    }

    flatcc_builder_reset(builder);

    return slot;
}

extern "C" TupleTableSlot *gaia_exec_foreign_update(
    EState *estate,
    ResultRelInfo *rinfo,
    TupleTableSlot *slot,
    TupleTableSlot *plan_slot) {
    // Update one tuple in the foreign table. estate is global execution state
    // for the query. rinfo is the ResultRelInfo struct describing the target
    // foreign table. slot contains the new data for the tuple; it will match
    // the rowtype definition of the foreign table. plan_slot contains the
    // tuple that was generated by the ModifyTable plan node's subplan; it
    // differs from slot in possibly containing additional "junk" columns. In
    // particular, any junk columns that were requested by
    // AddForeignUpdateTargets will be available from this slot.
    //
    // The return value is either a slot containing the row as it was actually
    // updated (this might differ from the data supplied, for example as a
    // result of trigger actions), or NULL if no row was actually updated
    // (again, typically as a result of triggers). The passed-in slot can be
    // re-used for this purpose.
    //
    // The data in the returned slot is used only if the UPDATE query has a
    // RETURNING clause. Hence, the FDW could choose to optimize away
    // returning some or all columns depending on the contents of the
    // RETURNING clause. However, some slot must be returned to indicate
    // success, or the query's reported rowcount will be wrong.
    //
    // If the ExecForeignUpdate pointer is set to NULL, attempts to update the
    // foreign table will fail with an error message.
    elog(DEBUG1, "Entering function %s...", __func__);

    gaia_fdw_modify_state_t *modify_state = (gaia_fdw_modify_state_t *)rinfo->ri_FdwState;
    flatcc_builder_t *builder = &modify_state->builder;
    modify_state->initializer(builder);

    // NB: we assume 0 is a valid sentinel value, i.e., it can never be a
    // system-generated gaia_id. This is true for our internal implementation,
    // but also needs to hold for all other implementations (until the storage
    // engine itself starts generating gaia_ids).
    uint64_t gaia_id = 0;

    // slot_getallattrs() is necessary beginning in Postgres 12 (the slot will
    // be empty!).
    // TODO: use slot_getattr()?
    slot_getallattrs(slot);
    for (int attr_idx = 0; attr_idx < slot->tts_tupleDescriptor->natts; attr_idx++) {
        if (!(slot->tts_isnull[attr_idx])) {
            attribute_builder_fn attr_builder
                = modify_state->indexed_builders[attr_idx];

            Datum attr_val = slot->tts_values[attr_idx];
            if (attr_idx == modify_state->pk_attr_idx) {
                gaia_id = DatumGetUInt64(attr_val);
            }

            attr_builder(builder, attr_val);
        }
    }

    // We must have found a valid (i.e., nonzero) gaia_id attribute value.
    assert(gaia_id);

    modify_state->finalizer(builder);

    size_t size;
    const void *buffer = flatcc_builder_get_direct_buffer(builder, &size);

    try {
        auto node = gaia_ptr::open(gaia_id);
        node.update_payload(size, buffer);
    } catch (const std::exception &e) {
        ereport(ERROR,
            (errcode(ERRCODE_FDW_ERROR),
            errmsg("Error updating gaia object."),
            errhint(e.what())));
    }

    flatcc_builder_reset(builder);

    return slot;
}

extern "C" TupleTableSlot *gaia_exec_foreign_delete(
    EState *estate,
    ResultRelInfo *rinfo,
    TupleTableSlot *slot,
    TupleTableSlot *plan_slot) {
    // Delete one tuple from the foreign table. estate is global execution
    // state for the query. rinfo is the ResultRelInfo struct describing the
    // target foreign table. slot contains nothing useful upon call, but can
    // be used to hold the returned tuple. plan_slot contains the tuple that
    // was generated by the ModifyTable plan node's subplan; in particular, it
    // will carry any junk columns that were requested by
    // AddForeignUpdateTargets. The junk column(s) must be used to identify
    // the tuple to be deleted.
    //
    // The return value is either a slot containing the row that was deleted,
    // or NULL if no row was deleted (typically as a result of triggers). The
    // passed-in slot can be used to hold the tuple to be returned.
    //
    // The data in the returned slot is used only if the DELETE query has a
    // RETURNING clause. Hence, the FDW could choose to optimize away
    // returning some or all columns depending on the contents of the
    // RETURNING clause. However, some slot must be returned to indicate
    // success, or the query's reported rowcount will be wrong.
    //
    // If the ExecForeignDelete pointer is set to NULL, attempts to delete
    // from the foreign table will fail with an error message.

    elog(DEBUG1, "Entering function %s...", __func__);

    TupleTableSlot *return_slot = slot;
    gaia_fdw_modify_state_t *modify_state = (gaia_fdw_modify_state_t *)rinfo->ri_FdwState;
    // Relation rel = rinfo->ri_RelationDesc;
    // Oid foreignTableId = RelationGetRelid(rel);

    // Get primary key (gaia_id) from plan slot.
    TupleDesc tupleDesc = plan_slot->tts_tupleDescriptor;
    // plan_slot should have only 1 attr (gaia_id).
    assert(tupleDesc->natts == 1);

    Form_pg_attribute attr = TupleDescAttr(tupleDesc, 0);
    AttrNumber attnum = attr->attnum;
    char *attr_name = NameStr(attr->attname);
    assert(strcmp("gaia_id", attr_name) == 0);

    bool is_null;
    Datum pk_val = slot_getattr(plan_slot, attnum, &is_null);
    assert(!is_null);
    uint64_t gaia_id = DatumGetUInt64(pk_val);

    try {
        modify_state->target_node = gaia_ptr::open(gaia_id);

        if (modify_state->target_node) {
            elog(DEBUG1, "Calling remove() on node for gaia_id %d.",
                gaia_id);
            gaia_ptr::remove(modify_state->target_node);
        } else {
            elog(DEBUG1, "Node for gaia_id %d is invalid.", gaia_id);
            return_slot = NULL;
        }
    } catch (const std::exception &e) {
        ereport(ERROR,
            (errcode(ERRCODE_FDW_ERROR),
            errmsg("Error deleting gaia object."), errhint(e.what())));
    }

    return return_slot;
}

extern "C" void gaia_end_foreign_modify(EState *estate, ResultRelInfo *rinfo) {
    // End the table update and release resources. It is normally not
    // important to release palloc'd memory, but for example open files and
    // connections to remote servers should be cleaned up.
    //
    // If the EndForeignModify pointer is set to NULL, no action is taken
    // during executor shutdown.
    elog(DEBUG1, "Entering function %s...", __func__);

    gaia_fdw_modify_state_t *modify_state = (gaia_fdw_modify_state_t *)rinfo->ri_FdwState;

    flatcc_builder_clear(&modify_state->builder);

    // For DELETE, this seems to always be called before EndForeignScan.
    commit_gaia_txn();
}

extern "C" void gaia_begin_foreign_insert(ModifyTableState *mtstate,
    ResultRelInfo *resultRelInfo) {
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" void gaia_end_foreign_insert(EState *estate,
    ResultRelInfo *resultRelInfo) {
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" int gaia_is_foreign_rel_updatable(Relation rel) {
    // Report which update operations the specified foreign table supports.
    // The return value should be a bit mask of rule event numbers indicating
    // which operations are supported by the foreign table, using the CmdType
    // enumeration; that is, (1 << CMD_UPDATE) = 4 for UPDATE, (1 <<
    // CMD_INSERT) = 8 for INSERT, and (1 << CMD_DELETE) = 16 for DELETE.
    //
    // If the IsForeignRelUpdatable pointer is set to NULL, foreign tables are
    // assumed to be insertable, updatable, or deletable if the FDW provides
    // ExecForeignInsert, ExecForeignUpdate, or ExecForeignDelete
    // respectively. This function is only needed if the FDW supports some
    // tables that are updatable and some that are not. (Even then, it's
    // permissible to throw an error in the execution routine instead of
    // checking in this function. However, this function is used to determine
    // updatability for display in the information_schema views.)
    elog(DEBUG1, "Entering function %s...", __func__);

    return (1 << CMD_UPDATE) | (1 << CMD_INSERT) | (1 << CMD_DELETE);
}

extern "C" bool gaia_plan_direct_modify(
    PlannerInfo *root,
    ModifyTable *plan,
    Index result_relation,
    int subplan_index) {
    elog(DEBUG1, "Entering function %s...", __func__);

    return false;
}

extern "C" void gaia_begin_direct_modify(ForeignScanState *node, int eflags) {
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" TupleTableSlot *gaia_iterate_direct_modify(ForeignScanState *node) {
    elog(DEBUG1, "Entering function %s...", __func__);
    return NULL;
}

extern "C" void gaia_end_direct_modify(ForeignScanState *node) {
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" void gaia_explain_foreign_scan(
    ForeignScanState *node,
    struct ExplainState *es) {
    // Print additional EXPLAIN output for a foreign table scan. This function
    // can call ExplainPropertyText and related functions to add fields to the
    // EXPLAIN output. The flag fields in es can be used to determine what to
    // print, and the state of the ForeignScanState node can be inspected to
    // provide run-time statistics in the EXPLAIN ANALYZE case.
    //
    // If the ExplainForeignScan pointer is set to NULL, no additional
    // information is printed during EXPLAIN.
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" void gaia_explain_foreign_modify(
    ModifyTableState *mtstate,
    ResultRelInfo *rinfo,
    List *fdw_private,
    int subplan_index,
    struct ExplainState *es) {
    // Print additional EXPLAIN output for a foreign table update. This
    // function can call ExplainPropertyText and related functions to add
    // fields to the EXPLAIN output. The flag fields in es can be used to
    // determine what to print, and the state of the ModifyTableState node can
    // be inspected to provide run-time statistics in the EXPLAIN ANALYZE
    // case. The first four arguments are the same as for BeginForeignModify.
    //
    // If the ExplainForeignModify pointer is set to NULL, no additional
    // information is printed during EXPLAIN.
    elog(DEBUG1, "Entering function %s...", __func__);

    // gaiaFdwModifyState *modify_state = (gaiaFdwModifyState *) rinfo->ri_FdwState;
}

extern "C" void gaia_explain_direct_modify(
    ForeignScanState *node,
    struct ExplainState *es) {
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" bool gaia_analyze_foreign_table(
    Relation relation,
    AcquireSampleRowsFunc *func,
    BlockNumber *total_pages) {
    // This function is called when ANALYZE is executed on a foreign table. If
    // the FDW can collect statistics for this foreign table, it should return
    // true, and provide a pointer to a function that will collect sample rows
    // from the table in func, plus the estimated size of the table in pages
    // in total_pages. Otherwise, return false.
    //
    // If the FDW does not support collecting statistics for any tables, the
    // AnalyzeForeignTable pointer can be set to NULL.
    //
    // If provided, the sample collection function must have the signature:
    //
    //    int
    //    AcquireSampleRowsFunc (Relation relation, int elevel,
    //                           HeapTuple *rows, int targ_rows,
    //                           double *total_rows,
    //                           double *total_dead_rows);
    //
    // A random sample of up to targrows rows should be collected from the
    // table and stored into the caller-provided rows array. The actual number
    // of rows collected must be returned. In addition, store estimates of the
    // total numbers of live and dead rows in the table into the output
    // parameters totalrows and totaldeadrows. (Set totaldeadrows to zero if
    // the FDW does not have any concept of dead rows.)
    elog(DEBUG1, "Entering function %s...", __func__);

    return false;
}

extern "C" void gaia_get_foreign_join_paths(
    PlannerInfo *root,
    RelOptInfo *join_rel,
    RelOptInfo *outer_rel,
    RelOptInfo *inner_rel,
    JoinType join_type,
    JoinPathExtraData *extra) {
    // Create possible access paths for a join of two (or more) foreign tables
    // that all belong to the same foreign server. This optional function is
    // called during query planning. As with GetForeignPaths, this function
    // should generate ForeignPath path(s) for the supplied join_rel, and call
    // add_path to add these paths to the set of paths considered for the
    // join. But unlike GetForeignPaths, it is not necessary that this
    // function succeed in creating at least one path, since paths involving
    // local joining are always possible.
    //
    // Note that this function will be invoked repeatedly for the same join
    // relation, with different combinations of inner and outer relations; it
    // is the responsibility of the FDW to minimize duplicated work.
    //
    // If a ForeignPath path is chosen for the join, it will represent the
    // entire join process; paths generated for the component tables and
    // subsidiary joins will not be used. Subsequent processing of the join
    // path proceeds much as it does for a path scanning a single foreign
    // table. One difference is that the scanrelid of the resulting
    // ForeignScan plan node should be set to zero, since there is no single
    // relation that it represents; instead, the fs_relids field of the
    // ForeignScan node represents the set of relations that were joined. (The
    // latter field is set up automatically by the core planner code, and need
    // not be filled by the FDW.) Another difference is that, because the
    // column list for a remote join cannot be found from the system catalogs,
    // the FDW must fill fdw_scan_tlist with an appropriate list of
    // TargetEntry nodes, representing the set of columns it will supply at
    // runtime in the tuples it returns.
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" void gaia_get_foreign_upper_paths(
    PlannerInfo *root,
    UpperRelationKind stage,
    RelOptInfo *input_rel,
    RelOptInfo *output_rel,
    void *extra) {
    elog(DEBUG1, "Entering function %s...", __func__);
}

extern "C" bool gaia_recheck_foreign_scan(
    ForeignScanState *node,
    TupleTableSlot *slot) {
    elog(DEBUG1, "Entering function %s...", __func__);

    return false;
}

extern "C" RowMarkType gaia_get_foreign_row_mark_type(
    RangeTblEntry *rte,
    LockClauseStrength strength) {
    // Report which row-marking option to use for a foreign table. rte is the
    // RangeTblEntry node for the table and strength describes the lock
    // strength requested by the relevant FOR UPDATE/SHARE clause, if any. The
    // result must be a member of the RowMarkType enum type.
    //
    // This function is called during query planning for each foreign table
    // that appears in an UPDATE, DELETE, or SELECT FOR UPDATE/SHARE query and
    // is not the target of UPDATE or DELETE.
    //
    // If the GetForeignRowMarkType pointer is set to NULL, the ROW_MARK_COPY
    // option is always used. (This implies that RefetchForeignRow will never
    // be called, so it need not be provided either.)
    elog(DEBUG1, "Entering function %s...", __func__);

    return ROW_MARK_COPY;
}

extern "C" void gaia_refetch_foreign_row(
    EState *estate,
    ExecRowMark *erm,
    Datum rowid,
    TupleTableSlot *slot,
    bool *updated) {
    // Re-fetch one tuple from the foreign table, after locking it if
    // required. estate is global execution state for the query. erm is the
    // ExecRowMark struct describing the target foreign table and the row lock
    // type (if any) to acquire. rowid identifies the tuple to be fetched.
    // updated is an output parameter.
    //
    // This function should return a palloc'ed copy of the fetched tuple, or
    // NULL if the row lock couldn't be obtained. The row lock type to acquire
    // is defined by erm->markType, which is the value previously returned by
    // GetForeignRowMarkType. (ROW_MARK_REFERENCE means to just re-fetch the
    // tuple without acquiring any lock, and ROW_MARK_COPY will never be seen
    // by this routine.)
    //
    // In addition, *updated should be set to true if what was fetched was an
    // updated version of the tuple rather than the same version previously
    // obtained. (If the FDW cannot be sure about this, always returning true
    // is recommended.)
    //
    // Note that by default, failure to acquire a row lock should result in
    // raising an error; a NULL return is only appropriate if the SKIP LOCKED
    // option is specified by erm->waitPolicy.
    //
    // The rowid is the ctid value previously read for the row to be
    // re-fetched. Although the rowid value is passed as a Datum, it can
    // currently only be a tid. The function API is chosen in hopes that it
    // may be possible to allow other datatypes for row IDs in future.
    //
    // If the RefetchForeignRow pointer is set to NULL, attempts to re-fetch
    // rows will fail with an error message.
    elog(DEBUG1, "Entering function %s...", __func__);
}

// We may want to use IMPORT FOREIGN SCHEMA to automatically create the foreign
// tables corresponding to registered types in the storage engine.
extern "C" List *gaia_import_foreign_schema(
    ImportForeignSchemaStmt *stmt,
    Oid server_oid) {
    // Obtain a list of foreign table creation commands. This function is
    // called when executing IMPORT FOREIGN SCHEMA, and is passed the parse
    // tree for that statement, as well as the OID of the foreign server to
    // use. It should return a list of C strings, each of which must contain a
    // CREATE FOREIGN TABLE command. These strings will be parsed and executed
    // by the core server.
    //
    // Within the ImportForeignSchemaStmt struct, remote_schema is the name of
    // the remote schema from which tables are to be imported. list_type
    // identifies how to filter table names: FDW_IMPORT_SCHEMA_ALL means that
    // all tables in the remote schema should be imported (in this case
    // table_list is empty), FDW_IMPORT_SCHEMA_LIMIT_TO means to include only
    // tables listed in table_list, and FDW_IMPORT_SCHEMA_EXCEPT means to
    // exclude the tables listed in table_list. options is a list of options
    // used for the import process. The meanings of the options are up to the
    // FDW. For example, an FDW could use an option to define whether the NOT
    // NULL attributes of columns should be imported. These options need not
    // have anything to do with those supported by the FDW as database object
    // options.
    //
    // The FDW may ignore the local_schema field of the
    // ImportForeignSchemaStmt, because the core server will automatically
    // insert that name into the parsed CREATE FOREIGN TABLE commands.
    //
    // The FDW does not have to concern itself with implementing the filtering
    // specified by list_type and table_list, either, as the core server will
    // automatically skip any returned commands for tables excluded according
    // to those options. However, it's often useful to avoid the work of
    // creating commands for excluded tables in the first place. The function
    // IsImportableForeignTable() may be useful to test whether a given
    // foreign-table name will pass the filter.
    elog(DEBUG1, "Entering function %s...", __func__);

    ForeignServer *server = GetForeignServer(server_oid);
    const char *server_name = server->servername;

    List *commands = NIL;
    const char *ddl_stmt_fmts[] = {
        c_airport_ddl_stmt_fmt,
        c_airline_ddl_stmt_fmt,
        c_route_ddl_stmt_fmt,
        c_event_log_ddl_stmt_fmt,
    };

    for (size_t i = 0; i < array_size(ddl_stmt_fmts); i++) {
        // Length of format string + length of server name - 2 chars for format
        // specifier + 1 char for null terminator.
        size_t stmt_len = strlen(ddl_stmt_fmts[i]) + strlen(server_name) - 2 + 1;
        char *stmt_buf = (char *)palloc(stmt_len);

        // sprintf returns number of chars written, not including null
        // terminator.
        if (sprintf(stmt_buf, ddl_stmt_fmts[i], server_name)
            != (int)(stmt_len - 1)) {
            elog(ERROR, "Failed to format statement '%s' with server name '%s'.",
                ddl_stmt_fmts[i], server_name);
        }

        commands = lappend(commands, stmt_buf);
    }

    return commands;
}

// Perform all module-level initialization here.
extern "C" void _PG_init() {
    elog(DEBUG1, "Entering function %s...", __func__);
    // Initialize COW-SE without deleting all data.
    gaia::db::begin_session();
}

// Perform all module-level finalization here.
extern "C" void _PG_fini() {
    elog(DEBUG1, "Entering function %s...", __func__);
    gaia::db::end_session();
}
