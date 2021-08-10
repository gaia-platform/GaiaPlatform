/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#pragma once

#include <iostream>

namespace gaia
{
namespace translation
{
// Strings used to build the version string
static const char c_gaiat[] = "Gaia Translation Engine";
static const char c_copyright[] = "Copyright (c) Gaia Platform LLC";

// Error messages.  These error messages are output to cerr during the process of 
// translating a ruleset file. Error messages may have additional runtime context 
// provided in the form of parameters. Parameters are substituted in place of % in 
// the error messages via the 'diagnostic_t' class.
// All errors should have the form of: <error cause> followed by <suggested remedy>.
static const char c_err_active_field_not_supported[] = 
    "Since one of 'on_insert', 'on_change', or 'on_update' was provided as part of the rule definition, "
    "specifying active fields using '@' inside the rule is not supported. Ensure you are specifying active "
    "fields using either the '@' sign in the rule or one of the rule definition attributes.";
static const char c_err_ambiguous_path[] = 
    "Ambiguous navigation path detected. There is more than one field that relates '%' to '%'. "
    "Specify the navigation path explicitly to resolve this ambiguity.";
// TODO: in this case we'd like the user to follow up with us and report the exception.  Do
// we have a mechanism for this?  I.e., "email support@gaiaplatform.io"?
static const char c_err_catalog_exception[] = 
    "An exception has occurred while processing the catlog: '%'.";
static const char c_err_duplicate_field[] = 
    "Duplicate field '%' found in table '%'. "
    "Qualify your with field with the table name (table.field) to disambiguate "
    "field names that occur in more than one table.";
static const char c_err_duplicate_ruleset[] = 
    "Ruleset '%' has been found multiple times. "
    "Ensure every ruleset has a unique name.";
//TODO how does the user get this error?
static const char c_err_edc_init[] = 
    "Initialization of a declared variable with an EDC object is not supported.";
static const char c_err_field_deprecated[] = 
    "Field '%' of table '%' is marked as deprecated in the catalog. "
    "Ensure you are using the correct field or unmark the field as being deprecated in the catalog.";
static const char c_err_field_hidden[] = 
    "Local variable '%' hides a database field with the same name. "
    "Rename your local variable so that it doesn't conflict with an existing database field name.";
static const char c_err_field_not_found[] = 
    "Field '%' of table '%' was not found in the database. "
    "Ensure your field and table names are correct in your rule.";
// TODO: how does the user correct these 'incorrect' errors?
static const char c_err_incorrect_base_type[] = "Incorrect base type of generated type.";
static const char c_err_incorrect_child_table[] = "Incorrect child table in the relationship '%'.";
static const char c_err_incorrect_table[] = "Incorrect table for field '%'.";
static const char c_err_incorrect_for_expression[] = 
    "Incorrect expression is used as a path in a 'for' statement. "
    "An expression in a 'for' statement must only iterate over a table.";
static const char c_err_incorrect_matched_expression[] = "Incorrect matched expression.";
static const char c_err_incorrect_matched_operator[] = "Incorrect matched operator.";
static const char c_err_incorrect_operator_expression[] = "Incorrect operator expression.";
static const char c_err_incorrect_operator_expression_type[] = "Incorrect operator expression type.";
static const char c_err_incorrect_operator_code[] = "Incorrect operator code '%'.";
static const char c_err_incorrect_operator_type[] = "Incorrect operator type.";
static const char c_err_incorrect_parent_table[] = "Incorrect parent table in the relationship '%'.";
static const char c_err_insert_with_explicit_nav[] = 
    "The 'insert' method cannot be used with an explicit navigation path. "
    "Remove the navigation path to the 'insert' method. A new row is not reachable from a table until after it is inserted.";
static const char c_err_insert_with_tag[] = 
    "The 'insert' method cannot be used with a tag. Remove the tag from the table in the 'insert' statement.";
static const char c_err_multi_anchor_tables[] = 
    "A rule may not specify multiple tables or active fields from different tables in 'on_insert', "
    "'on_change', or 'on_update'. "
    "Ensure all your active field references belong to the same table.";
static const char c_err_multi_anchor_fields[] = 
    "A rule may not specify active fields from different tables. "
    "Ensure all your active field references belong to the same table.";
static const char c_err_multiple_ruleset_files[] = 
    "The Translation Engine does not support more than one source ruleset file. "
    "Combine your rulesets into a single ruleset file.";
static const char c_err_multiple_shortest_paths[] =
    "Multiple shortest paths exist between tables '%' and '%'. "
    "Explicitly specify the navigation path between these two tables to resolve the ambiguity.";
// TODO:  how do I get to this error?
static const char c_err_no_fields_referenced_by_table[] = "No fields referenced by table '%'.";
static const char c_err_no_active_fields[] = 
    "No active fields were referenced in this rule. "
    "Ensure your rule references at least one active field or table.";
static const char c_err_no_anchor_path[] = 
    "No navigation path exists between table '%' and other tables. "
    "Ensure your schema has a relationship between the tables you are referencing.";
static const char c_err_no_path[] = 
    "No navigation path exists between tables '%' and '%'. "
    "Ensure your schema has a relationship between these two tables.";
// TODO: is this error the same as the one above from a user's perspective?
static const char c_err_no_relationship[] = 
    "No relationship exists between tables '%' and '%'. "
    "Ensure your schema has a relationship between these two tables.";
static const char c_err_table_hidden[] = 
    "Local variable '%' hides a database table with the same name. "
    "Rename your local variable so that it doesn't conflict with an existing database table name.";
//  TODO:  catalog or database?
static const char c_err_table_not_found[] = 
    "Table '%' was not found in the catalog. "
    "Ensure the table you are referencing in your rule exists in the database.";
static const char c_err_tag_hidden[] = 
    "Local variable '%' hides a tag defined with the same name. "
    "Rename your local variable or tag so that their names do not conflict.";

// This class allows streaming a single error message with an arbitrary number of parameters
// out to std::err based on the error format strings defined above.
//
// Calling this class directly is supported, however, using the `diag_err` helper function
// makes it syntactically easier to use this class with diagnostic messages that have
// no arguments.  See https://stackoverflow.com/questions/30209962/how-to-create-temporary-object-in-c for
// more details.
class diagnostic_t
{
public:
    diagnostic_t() = delete;
    diagnostic_t(const diagnostic_t &) = delete;
    void operator=(const diagnostic_t&) = delete;

    diagnostic_t(const char* message);
    ~diagnostic_t();

    // If you want to support another argument type then
    // add a stream overload here and add a template specialization
    // of output_next_arg in diagnostics.cpp.
    diagnostic_t& operator<<(const std::string& arg);
    diagnostic_t& operator<<(int arg);

private:
    template<typename T_arg>
    void output_next_arg(T_arg arg);

    const char* m_message;
};

// Helper method to emit an err using a diagnostic_t class
// under the covers.  Example usage:
// diag_err(c_err_no_params);
// diag_err(c_err_one_param) << first_param;
// diag_err(c_err_two_params) << first_param << second_param;
diagnostic_t diag_err(const char* message);

} // namespace translation
} // namespace gaia
