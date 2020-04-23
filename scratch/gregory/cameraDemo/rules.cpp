#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"
#include <stdio.h>
using namespace gaia::rules;

std::vector<string> processImage(const char *fileName);

/** ruleset*/
namespace cameraDemo
{
/**
 rule-image_create: [CameraDemo::Camera_image](insert)
*/
    void ImageCreate_handler(const rule_context_t *context)
    {        
        CameraDemo::Camera_image* row = static_cast<CameraDemo::Camera_image*>(context->event_context);
        cerr << "IMAGE Captured " << row->file_name() <<  endl;

        std::vector<string> detectedClasses  = processImage(row->file_name());
        if (detectedClasses.empty())
        {
            row->delete_row();
        }
        else
        {
            for (auto it = detectedClasses.cbegin(); it != detectedClasses.cend(); ++it)
            {
                CameraDemo::Object obj;
                obj.set_class_(it->c_str());
                obj.insert_row();
            }
        }        
    }

/**
 rule-image_delete: [CameraDemo::Camera_image](delete)
*/
    void ImageDelete_handler(const rule_context_t *context)
    {
        CameraDemo::Camera_image* row = static_cast<CameraDemo::Camera_image*>(context->event_context);
        ::remove(row->file_name());
        cerr << "IMAGE deleted " <<  row->file_name() << endl;
    }

/**
 rule-object_class: [CameraDemo::Object](insert)
*/
    void ObjectClassify_handler(const rule_context_t *context)
    {
        CameraDemo::Object* row = static_cast<CameraDemo::Object*>(context->event_context);
        cerr << "OBJECT CLASSIFIED " << row->class_() << endl;
    }
} 
