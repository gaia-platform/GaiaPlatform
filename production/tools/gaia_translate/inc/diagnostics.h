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
// the error messages via the print_error functions.
// All errors should have the form of: <error cause> followed by <suggested remedy>.
static const char c_err_active_field_not_supported[] = 
    "Since 'on_insert', 'on_change', or 'on_update' was provided, specifying active fields using '@' "
    "inside the rule is not supported.  Use one of the 'on_insert', 'on_change', 'on_update' rule "
    "attributes or specify active fields using '@' in the rule.";
static const char c_err_ambiguous_path[] = 
    "Ambiguous navigation path detected. There is more than one field that relates '%' to '%'. "
    "Specify the navigation path explicitly to resolve this ambiguity.";
// TODO: in this case we'd like the user to follow up with us and report the exception.  Do
// we have a mechanism for this?  I.e., "email support@gaiaplatform.io"?
static const char c_err_catalog_exception[] = 
    "An exception has occurred while processing the catlog: '%'.";
static const char c_err_duplicate_field[] = 
    "Duplicate field '%' found in table '%'.";
static const char c_err_duplicate_ruleset[] = 
    "Ruleset names must be unique. Ruleset '%' has been found multiple times."
    "Ensure each of your ruleset declarations have a unique name.";
//TODO how does the user get this error?
static const char c_err_edc_init[] = 
    "Initialization of a declared variable with an EDC object is not supported.";
static const char c_err_field_deprecated[] = 
    "Field '%' of table '%' is marked as deprecated in the catalog."
    "Either choose a different field or verify that the field should not be deprecated.";
static const char c_err_field_hidden[] = 
    "Local variable declaration '%' hides a database field defined with the same name."
    "Rename your local variable declaration so that it doesn't conflict with an existing database field name in the current scope.";
static const char c_err_field_not_found[] = 
    "Field '%' of table '%' was not found in the catalog."
    "Double-check your schema to ensure the naming of fields and tables are correct.";
// TODO: how does the user correct these 'incorrect' errors?
static const char c_err_incorrect_base_type[] = "Incorrect base type of generated type.";
static const char c_err_incorrect_child_table[] = "Incorrect child table in the relationship '%'.";
static const char c_err_incorrect_table[] = "Incorrect table for field '%'.";
static const char c_err_incorrect_for_expression[] = 
    "Incorrect expression is used as a path in a 'for' statement."
    "An expression in a 'for' statement must only iterate over a table.";
static const char c_err_incorrect_matched_expression[] = "Incorrect matched expression.";
static const char c_err_incorrect_matched_operator[] = "Incorrect matched operator.";
static const char c_err_incorrect_operator_expression[] = "Incorrect operator expression.";
static const char c_err_incorrect_operator_expression_type[] = "Incorrect operator expression type.";
static const char c_err_incorrect_operator_code[] = "Incorrect operator code '%'.";
static const char c_err_incorrect_operator_type[] = "Incorrect operator type.";
static const char c_err_incorrect_parent_table[] = "Incorrect parent table in the relationship '%'.";
static const char c_err_insert_with_explicit_nav[] = 
    "The 'insert' method cannot be used in an explicit navigation path."
    "Remove the navigation path to the 'insert' method. A new row is not reachable from table until after it is inserted.";
static const char c_err_insert_with_tag[] = 
    "The 'insert' method cannot be used with a tag. Remove the tag from the table off of which the 'insert' method is called.";
static const char c_err_multi_anchor_tables[] = 
    "A rule may not specify multiple tables or active fields from different tables in "
    "'on_insert', 'on_change', or 'on_update'. Ensure all your active field references belong to the same table.";
static const char c_err_multi_anchor_fields[] = 
    "A rule may not specify active fields from different tables. Ensure all your active field references belong to the same table.";
static const char c_err_multiple_ruleset_files[] = 
    "The Translation Engine does not support more than one source ruleset file.  Combine your rulesets into a single ruleset file.";
static const char c_err_multiple_shortest_paths[] =
    "Multiple shortest paths exist between tables '%' and '%'. "
    "Explicitly define the navigation path between these two tables to resolve the ambiguity.";
// TODO:  how do I get to this error?
static const char c_err_no_fields_referenced_by_table[] = "No fields referenced by table '%'.";
static const char c_err_no_active_fields[] = 
    "No active fields were referenced in this rule.  Ensure your rule references at least one active field or table.";
static const char c_err_no_anchor_path[] = 
    "No navigation path exists between table '%' and other tables. "
    "Ensure your schema has defined a relationship between the tables you are referencing.";
static const char c_err_no_path[] = 
    "No navigation path exists between tables '%' and '%'. "
    "Ensure your schema has defined a relationship between these two tables.";
// TODO: is this error the same as the one above from a user's perspective?
static const char c_err_no_relationship[] = 
    "No relationship exists between tables '%' and '%'. "
    "Ensure your schema has defined a relationship between these two tables.";
static const char c_err_table_hidden[] = 
    "Local variable declaration '%' hides a database table defined with the same name. "
    "Rename your local variable declaration so that it doesn't conflict with an existing database table name.";
//  TODO:  catalog or database?
static const char c_err_table_not_found[] = 
    "Table '%' was not found in the catalog.  Ensure the table you are referencing in your rule exists in the database.";
static const char c_err_tag_hidden[] = 
    "Local variable declaration '%' hides a tag defined with the same name."
    "Rename your local variable declaration or tag declaration so that they do not conflict.";

// Error print helpers.  The print_error method takes its template parameters
// and creates a tuple to pass to this internal method.  This method
// then calls itself recursively to process each parameter and output it as
// it encounters the placeholders in the error format string.
template <size_t T_index = 0, typename... T_args>
constexpr void print_error_internal(const char* format, std::tuple<T_args...> args)
{
    if constexpr(T_index == sizeof...(T_args))
    {
        // We have exhausted our parameters.  Print out what is
        // left of the format string.  Note that this case is also
        // for error strings with no parameters.
        std::cerr << format;
        return;
    }
    else
    {
        for(; *format != '\0'; format++)
        {
            if (*format == '%')
            {
                // Get the argument out of the tuple and then
                // move to the next one.
                std::cerr << std::get<T_index>(args);
                print_error_internal<T_index + 1>(format + 1, args);
                return;
            }
            std::cerr << *format;
        }
    }
}

template <typename... T_args>
void print_error(const char* format, T_args... args)
{
    std::cerr << "error: ";
    print_error_internal(format, std::tuple<T_args...>(args...));
    std::cerr << std::endl;
}

} // namespace translation
} // namespace gaia
