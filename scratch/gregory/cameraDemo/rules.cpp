#include <chrono>
#include <cstdio>

#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"

using namespace gaia::rules;
using namespace std::chrono;

std::vector<string> processImage(const char *fileName);

/** ruleset*/
namespace cameraDemo
{
/**
 rule-image_create: [CameraDemo::Camera_image](insert)
*/
    void ImageCreate_handler(const rule_context_t *context)
    {        
        auto row = static_cast<CameraDemo::Camera_image*>(
            context->event_context);
        cerr << "IMAGE Captured " << row->file_name() <<  endl;

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
        auto row = static_cast<CameraDemo::Camera_image*>(
            context->event_context);
        ::remove(row->file_name());
        cerr << "IMAGE deleted " <<  row->file_name() << endl;
    }

/**
 rule-object_class: [CameraDemo::Object](insert)
*/
    void ObjectClassify_handler(const rule_context_t *context)
    {
        auto row = static_cast<CameraDemo::Object*>(context->event_context);
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
