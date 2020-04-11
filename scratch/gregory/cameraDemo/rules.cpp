#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"
#include <stdio.h>
using namespace gaia::rules;

std::vector<string> processImage(const char *fileName);

/** ruleset*/
namespace cameraDemo
{
/**
 rule, image_create,CameraDemo::kCameraImageType=1,  event_type_t::row_insert
*/
    void ImageCreate_handler(const context_base_t *context)
    {        
        const table_context_t* t = static_cast<const table_context_t*>(context);
        CameraDemo::CameraImage * row = static_cast<CameraDemo::CameraImage*>(t->row);
        cerr << "IMAGE Captured " << row->fileName() <<  endl;

        std::vector<string> detectedClasses(processImage(row->fileName()));
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
 rule, image_delete,CameraDemo::kCameraImageType=1,  event_type_t::row_delete
*/
    void ImageDelete_handler(const context_base_t *context)
    {
        const table_context_t* t = static_cast<const table_context_t*>(context);
        CameraDemo::CameraImage * row = static_cast<CameraDemo::CameraImage*>(t->row);
        remove(row->fileName());
        cerr << "IMAGE deleted " <<  row->fileName() << endl;
    }

/**
 rule, object_class,CameraDemo::kObjectType=2,  event_type_t::row_insert
*/
    void ObjectClassify_handler(const context_base_t *context)
    {
        const table_context_t* t = static_cast<const table_context_t*>(context);
        CameraDemo::Object * row = static_cast<CameraDemo::Object*>(t->row);
        cerr << "OBJECT CLASSIFIED " << row->class_() << endl;
    }
} 
