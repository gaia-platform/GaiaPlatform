#include <chrono>
#include <iostream>
#include <map>
#include <string>

#include "rules.hpp"
#include "gaia_cameraDemo.h"

using namespace gaia::rules;
using namespace std::chrono;

std::map<gaia_id_t, std::string> g_filenames;
std::vector<string> processImage(const char *fileName);

namespace cameraDemo
{
  
void cameraDemo_MnnP9tkkD7OO52Z_1(const rule_context_t* context)
{
gaia::cameraDemo::camera_image_t camera_image = gaia::cameraDemo::camera_image_t::get(context->record);

        if (context->last_operation(gaia::cameraDemo::camera_image_t::s_gaia_type) == gaia::rules::last_operation_t::row_insert)
        {
            std::cerr << "IMAGE Captured " << camera_image.file_name() <<  endl;
            g_filenames.insert(make_pair(camera_image.gaia_id(), camera_image.file_name()));
            std::vector<string> detectedClasses  = processImage(camera_image.file_name());
            if (detectedClasses.empty())
            {
                camera_image.delete_row();
            }
            else
            {
                for (const auto& it: detectedClasses)
                {
                    gaia::cameraDemo::object_writer obj = gaia::cameraDemo::object_writer();
                    obj.object_class = it.c_str();
                    obj.insert_row();
                }
            } 
        }      
    }

    
void cameraDemo_sxKd1S3J6OgObPA_1(const rule_context_t* context)
{
gaia::cameraDemo::camera_image_t camera_image = gaia::cameraDemo::camera_image_t::get(context->record);

    	if (context->last_operation(gaia::cameraDemo::camera_image_t::s_gaia_type) == gaia::rules::last_operation_t::row_delete)
    	{
        auto file_name = g_filenames[camera_image.gaia_id()];
        ::remove(file_name.c_str());
        
        cerr << "IMAGE deleted " <<  file_name << endl;
        g_filenames.erase(camera_image.gaia_id());
        }
    }

    
void cameraDemo_Wxn0lUDfQC88fPE_1(const rule_context_t* context)
{
gaia::cameraDemo::object_t object = gaia::cameraDemo::object_t::get(context->record);

        if (context->last_operation(gaia::cameraDemo::object_t::s_gaia_type) == gaia::rules::last_operation_t::row_insert)
        {
            cerr << "OBJECT CLASSIFIED " << object.object_class() << endl;

            if ("person" == std::string(object.object_class()))
            {
                auto until_time_ns = static_cast<ulong>(
                    high_resolution_clock::now().time_since_epoch().count() +
                    duration_cast<nanoseconds>(seconds(3)).count());

                auto p_row = gaia::cameraDemo::emergency_stop_t::get_first();
                if (!p_row) {
                    gaia::cameraDemo::emergency_stop_writer w = gaia::cameraDemo::emergency_stop_writer();
                    w.until_time_ns = until_time_ns;
                    w.insert_row();
                }
                else
                {
                    gaia::cameraDemo::emergency_stop_writer w = p_row.writer();
                    w.until_time_ns = until_time_ns;
                    w.update_row();
                }
            }
        }
    }

    
void cameraDemo_kJwfIcszcTSC7Zc_1(const rule_context_t* context)
{
gaia::cameraDemo::emergency_stop_t emergency_stop = gaia::cameraDemo::emergency_stop_t::get(context->record);

    	if (context->last_operation(gaia::cameraDemo::emergency_stop_t::s_gaia_type) == gaia::rules::last_operation_t::row_insert)
    	{
        	cerr << "EMERGENCY STOP CREATED" << endl;
        }
    }


    
void cameraDemo_1cZlXR_dTejNYYH_1(const rule_context_t* context)
{
gaia::cameraDemo::emergency_stop_t emergency_stop = gaia::cameraDemo::emergency_stop_t::get(context->record);

    	if (context->last_operation(gaia::cameraDemo::emergency_stop_t::s_gaia_type) == gaia::rules::last_operation_t::row_update)
    	{
        	cerr << "EMERGENCY STOP UPDATED" << endl;
        }
    }

    
void cameraDemo_7JhV2b30uBlTOlA_1(const rule_context_t* context)
{
gaia::cameraDemo::camera_image_t camera_image = gaia::cameraDemo::camera_image_t::get(context->record);

    	if (context->last_operation(gaia::cameraDemo::camera_image_t::s_gaia_type) == gaia::rules::last_operation_t::row_insert)
    	{
            auto p_row =  gaia::cameraDemo::emergency_stop_t::get_first();
            if ( p_row
                && p_row.until_time_ns() < static_cast<ulong>(
                   high_resolution_clock::now().time_since_epoch().count()))
            {
                cerr << "EMERGENCY STOP EXPIRED" << endl;
            	 p_row.delete_row();
            }
        }
    }

} 
namespace cameraDemo{
void subscribeRuleset_cameraDemo()
{
rule_binding_t cameraDemo_MnnP9tkkD7OO52Z_1binding("cameraDemo","cameraDemo_MnnP9tkkD7OO52Z_1",cameraDemo::cameraDemo_MnnP9tkkD7OO52Z_1);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_MnnP9tkkD7OO52Z_1binding);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_MnnP9tkkD7OO52Z_1binding);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_MnnP9tkkD7OO52Z_1binding);
rule_binding_t cameraDemo_sxKd1S3J6OgObPA_1binding("cameraDemo","cameraDemo_sxKd1S3J6OgObPA_1",cameraDemo::cameraDemo_sxKd1S3J6OgObPA_1);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_sxKd1S3J6OgObPA_1binding);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_sxKd1S3J6OgObPA_1binding);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_sxKd1S3J6OgObPA_1binding);
rule_binding_t cameraDemo_Wxn0lUDfQC88fPE_1binding("cameraDemo","cameraDemo_Wxn0lUDfQC88fPE_1",cameraDemo::cameraDemo_Wxn0lUDfQC88fPE_1);
subscribe_rule(gaia::cameraDemo::object_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_Wxn0lUDfQC88fPE_1binding);
subscribe_rule(gaia::cameraDemo::object_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_Wxn0lUDfQC88fPE_1binding);
subscribe_rule(gaia::cameraDemo::object_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_Wxn0lUDfQC88fPE_1binding);
rule_binding_t cameraDemo_kJwfIcszcTSC7Zc_1binding("cameraDemo","cameraDemo_kJwfIcszcTSC7Zc_1",cameraDemo::cameraDemo_kJwfIcszcTSC7Zc_1);
subscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_kJwfIcszcTSC7Zc_1binding);
subscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_kJwfIcszcTSC7Zc_1binding);
subscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_kJwfIcszcTSC7Zc_1binding);
rule_binding_t cameraDemo_1cZlXR_dTejNYYH_1binding("cameraDemo","cameraDemo_1cZlXR_dTejNYYH_1",cameraDemo::cameraDemo_1cZlXR_dTejNYYH_1);
subscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_1cZlXR_dTejNYYH_1binding);
subscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_1cZlXR_dTejNYYH_1binding);
subscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_1cZlXR_dTejNYYH_1binding);
rule_binding_t cameraDemo_7JhV2b30uBlTOlA_1binding("cameraDemo","cameraDemo_7JhV2b30uBlTOlA_1",cameraDemo::cameraDemo_7JhV2b30uBlTOlA_1);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_7JhV2b30uBlTOlA_1binding);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_7JhV2b30uBlTOlA_1binding);
subscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_7JhV2b30uBlTOlA_1binding);
}
void unsubscribeRuleset_cameraDemo()
{
rule_binding_t cameraDemo_MnnP9tkkD7OO52Z_1binding("cameraDemo","cameraDemo_MnnP9tkkD7OO52Z_1",cameraDemo::cameraDemo_MnnP9tkkD7OO52Z_1);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_MnnP9tkkD7OO52Z_1binding);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_MnnP9tkkD7OO52Z_1binding);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_MnnP9tkkD7OO52Z_1binding);
rule_binding_t cameraDemo_sxKd1S3J6OgObPA_1binding("cameraDemo","cameraDemo_sxKd1S3J6OgObPA_1",cameraDemo::cameraDemo_sxKd1S3J6OgObPA_1);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_sxKd1S3J6OgObPA_1binding);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_sxKd1S3J6OgObPA_1binding);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_sxKd1S3J6OgObPA_1binding);
rule_binding_t cameraDemo_Wxn0lUDfQC88fPE_1binding("cameraDemo","cameraDemo_Wxn0lUDfQC88fPE_1",cameraDemo::cameraDemo_Wxn0lUDfQC88fPE_1);
unsubscribe_rule(gaia::cameraDemo::object_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_Wxn0lUDfQC88fPE_1binding);
unsubscribe_rule(gaia::cameraDemo::object_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_Wxn0lUDfQC88fPE_1binding);
unsubscribe_rule(gaia::cameraDemo::object_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_Wxn0lUDfQC88fPE_1binding);
rule_binding_t cameraDemo_kJwfIcszcTSC7Zc_1binding("cameraDemo","cameraDemo_kJwfIcszcTSC7Zc_1",cameraDemo::cameraDemo_kJwfIcszcTSC7Zc_1);
unsubscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_kJwfIcszcTSC7Zc_1binding);
unsubscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_kJwfIcszcTSC7Zc_1binding);
unsubscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_kJwfIcszcTSC7Zc_1binding);
rule_binding_t cameraDemo_1cZlXR_dTejNYYH_1binding("cameraDemo","cameraDemo_1cZlXR_dTejNYYH_1",cameraDemo::cameraDemo_1cZlXR_dTejNYYH_1);
unsubscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_1cZlXR_dTejNYYH_1binding);
unsubscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_1cZlXR_dTejNYYH_1binding);
unsubscribe_rule(gaia::cameraDemo::emergency_stop_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_1cZlXR_dTejNYYH_1binding);
rule_binding_t cameraDemo_7JhV2b30uBlTOlA_1binding("cameraDemo","cameraDemo_7JhV2b30uBlTOlA_1",cameraDemo::cameraDemo_7JhV2b30uBlTOlA_1);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,cameraDemo_7JhV2b30uBlTOlA_1binding);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,cameraDemo_7JhV2b30uBlTOlA_1binding);
unsubscribe_rule(gaia::cameraDemo::camera_image_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,cameraDemo_7JhV2b30uBlTOlA_1binding);
}
}
namespace gaia {
 namespace rules{
extern "C" void subscribe_ruleset(const char* ruleset_name)
{
if (strcmp(ruleset_name, "cameraDemo") == 0)
{
::cameraDemo::subscribeRuleset_cameraDemo();
return;
}
throw ruleset_not_found(ruleset_name);
}
extern "C" void unsubscribe_ruleset(const char* ruleset_name)
{
if (strcmp(ruleset_name, "cameraDemo") == 0)
{
::cameraDemo::unsubscribeRuleset_cameraDemo();
return;
}
throw ruleset_not_found(ruleset_name);
}
extern "C" void initialize_rules()
{
::cameraDemo::subscribeRuleset_cameraDemo();
}
}
}