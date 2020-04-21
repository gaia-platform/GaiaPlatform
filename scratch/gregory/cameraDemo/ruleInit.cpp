#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"

using namespace gaia::rules;

namespace cameraDemo{
void ObjectClassify_handler(const rule_context_t* context);
}
namespace cameraDemo{
void ImageCreate_handler(const rule_context_t* context);
}
namespace cameraDemo{
void ImageDelete_handler(const rule_context_t* context);
}

extern "C" void initialize_rules()
{
    rule_binding_t  ObjectClassify_handler("cameraDemo","rule-object_class",cameraDemo::ObjectClassify_handler);
    rule_binding_t  ImageDelete_handler("cameraDemo","rule-image_delete",cameraDemo::ImageDelete_handler);
    rule_binding_t  ImageCreate_handler("cameraDemo","rule-image_create",cameraDemo::ImageCreate_handler);

    subscribe_database_rule(CameraDemo::Camera_image::s_gaia_type, event_type_t::row_insert, ImageCreate_handler);

    subscribe_database_rule(CameraDemo::Camera_image::s_gaia_type, event_type_t::row_delete, ImageDelete_handler);

    subscribe_database_rule(CameraDemo::Object::s_gaia_type, event_type_t::row_insert, ObjectClassify_handler);

}
