#include <chrono>
#include <cstdio>
#include <map>
#include <string>

#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"

using namespace gaia::rules;
using namespace std::chrono;

std::map<gaia_id_t, std::string> g_filenames;

std::vector<string> processImage(const char *fileName);



/** ruleset*/
namespace cameraDemo
{
/**
 rule-image_create: [CameraDemo::Camera_image](insert)
*/
    void ImageCreate_handler(const rule_context_t *context)
    {
        auto row = CameraDemo::Camera_image::get_row_by_id(context->record);
        cerr << "IMAGE Captured " << row->file_name() <<  endl;
        g_filenames.insert(make_pair(row->gaia_id(), row->file_name()));
        std::vector<string> detectedClasses  = processImage(row->file_name());
        if (detectedClasses.empty())
        {
            row->delete_row();
        }
        else
        {
            for (const auto& it: detectedClasses)
            {
                CameraDemo::Object obj;
                obj.set_class_(it.c_str());
                obj.insert_row();
            }
        }        
    }

/**
 rule-image_delete: [CameraDemo::Camera_image](delete)
*/
    void ImageDelete_handler(const rule_context_t *context)
    {
        auto file_name = g_filenames[context->record];
        ::remove(file_name.c_str());
        
        cerr << "IMAGE deleted " <<  file_name << endl;
        g_filenames.erase(context->record);
    }

/**
 rule-object_class: [CameraDemo::Object](insert)
*/
    void ObjectClassify_handler(const rule_context_t *context)
    {
        auto row = CameraDemo::Object::get_row_by_id(context->record);
        cerr << "OBJECT CLASSIFIED " << row->class_() << endl;

        if ("person" == std::string(row->class_()))
        {
            auto until_time_ns = static_cast<ulong>(
                high_resolution_clock::now().time_since_epoch().count() +
                duration_cast<nanoseconds>(seconds(3)).count());

            auto p_row = CameraDemo::Emergency_stop::get_first();
            if (nullptr == p_row) {
                CameraDemo::Emergency_stop emergency_stop;
                emergency_stop.set_until_time_ns(until_time_ns);
                emergency_stop.insert_row();
            }
            else
            {
                p_row->set_until_time_ns(until_time_ns);
                p_row->update_row();
            }
        }
    }

/**
 rule-emergency_stop_create: [CameraDemo::Emergency_stop](insert)
*/
    void EmergencyStopCreate_handler(const rule_context_t *)
    {
        cerr << "EMERGENCY STOP CREATED" << endl;
    }

/**
 rule-emergency_stop_update: [CameraDemo::Emergency_stop](update)
*/
    void EmergencyStopUpdate_handler(const rule_context_t *)
    {
        cerr << "EMERGENCY STOP UPDATED" << endl;
    }

/**
 rule-emergency_stop_expire: [CameraDemo::Camera_image](insert)

 Use the Camera_image insert event to poll the Emergency_stop table. For any
 row in the table, if the "until_time_ns" field is in the past, delete the row.
*/
    void EmergencyStopExpire_handler(const rule_context_t *)
    {
        auto p_row = CameraDemo::Emergency_stop::get_first();
        if (nullptr != p_row
            && p_row->until_time_ns() < static_cast<ulong>(
                high_resolution_clock::now().time_since_epoch().count()))
        {
            cerr << "EMERGENCY STOP EXPIRED" << endl;
            p_row->delete_row();
        }
    }

}  // namepsace cameraDemo
