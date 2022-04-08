// Code generated by ObjectBox; DO NOT EDIT.

#pragma once

#ifdef __cplusplus
#include <cstdbool>
#include <cstdint>
extern "C" {
#else
#include <stdbool.h>
#include <stdint.h>
#endif
#include "objectbox.h"

/// Initializes an ObjectBox model for all entities. 
/// The returned pointer may be NULL if the allocation failed. If the returned model is not NULL, you should check if   
/// any error occurred by calling obx_model_error_code() and/or obx_model_error_message(). If an error occurred, you're
/// responsible for freeing the resources by calling obx_model_free().
/// In case there was no error when setting the model up (i.e. obx_model_error_code() returned 0), you may configure 
/// OBX_store_options with the model by calling obx_opt_model() and subsequently opening a store with obx_store_open().
/// As soon as you call obx_store_open(), the model pointer is consumed and MUST NOT be freed manually.
static inline OBX_model* create_obx_model() {
    OBX_model* model = obx_model();
    if (!model) return NULL;
    
    obx_model_entity(model, "Task", 1, 5838066383419397044);
    obx_model_property(model, "id", OBXPropertyType_Long, 1, 7328117824866458167);
    obx_model_property_flags(model, OBXPropertyFlags_ID);
    obx_model_property(model, "text", OBXPropertyType_String, 2, 6415399937195214707);
    obx_model_property(model, "date_created", OBXPropertyType_Long, 3, 7066512380192593760);
    obx_model_property_flags(model, OBXPropertyFlags_UNSIGNED);
    obx_model_property(model, "date_finished", OBXPropertyType_Long, 4, 8111127701577119000);
    obx_model_property_flags(model, OBXPropertyFlags_UNSIGNED);
    obx_model_entity_last_property_id(model, 4, 8111127701577119000);
    
    obx_model_entity(model, "simple_table", 2, 4047971602785158003);
    obx_model_property(model, "id", OBXPropertyType_Long, 1, 2015899393627004400);
    obx_model_property_flags(model, OBXPropertyFlags_ID);
    obx_model_property(model, "field", OBXPropertyType_Long, 2, 705668466319167817);
    obx_model_property_flags(model, OBXPropertyFlags_UNSIGNED);
    obx_model_entity_last_property_id(model, 2, 705668466319167817);
    
    obx_model_last_entity_id(model, 2, 4047971602785158003);
    return model; // NOTE: the returned model will contain error information if an error occurred.
}

#ifdef __cplusplus
}
#endif
