#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"
using namespace gaia::rules;

/** ruleset*/
namespace cameraDemo
{
/**
 rule, image_create,CameraDemo::kCameraImageType=1,  event_type_t::row_insert
*/
    void ImageCreate_handler(const context_base_t *context)
    {
        cerr << "q" << endl;
        const table_context_t* t = static_cast<const table_context_t*>(context);
        CameraDemo::CameraImage * row = static_cast<CameraDemo::CameraImage *>(t->row);
        cerr << "IMAGE Created " << row->fileName() <<  endl;
    }

/**
 rule, image_delete,CameraDemo::kCameraImageType=1,  event_type_t::row_delete
*/
    void ImageDelete_handler(const context_base_t *context)
    {
        cerr << "IMAGE deleted" << endl;
    }

/**
 rule, object_class,CameraDemo::kObjectType=2,  event_type_t::row_insert
*/
    void ObjectClassify_handler(const context_base_t *context)
    {
        cerr << "OBJECT CLASSIFIED" << endl;
    }
} 
