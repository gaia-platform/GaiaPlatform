#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"
using namespace gaia::rules;

/** ruleset*/
namespace cameraDemo
{
/**
 rule, image_create,CameraDemo::kCameraImageType,  event_type_t::row_insert
*/
    void ImageCreate_handler(const context_base_t *context)
    {
        const table_context_t* t = static_cast<const table_context_t*>(context);
        CameraDemo::CameraImage * row = static_cast<CameraDemo::CameraImage *>(t->row);
        cout << "IMAGE Created " << row->fileName() <<  endl;
    }

/**
 rule, image_delete,CameraDemo::kCameraImageType,  event_type_t::row_delete
*/
    void ImageDelete_handler(const context_base_t *context)
    {
        cout << "IMAGE deleted" << endl;
    }

/**
 rule, object_class,CameraDemo::kObjectType,  event_type_t::row_insert
*/
    void ObjectClassify_handler(const context_base_t *context)
    {
        cout << "OBJECT CLASSIFIED" << endl;
    }
} 
