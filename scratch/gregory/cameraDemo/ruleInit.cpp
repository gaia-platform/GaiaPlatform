#include "rules.hpp"
using namespace gaia::rules;
namespace cameraDemo{
void ObjectClassify_handler(const context_base_t *context);
}
namespace cameraDemo{
void ImageCreate_handler(const context_base_t *context);
}
namespace cameraDemo{
void ImageDelete_handler(const context_base_t *context);
}
namespace CameraDemo{
gaia::common::gaia_type_t kObjectType=2;
}
namespace CameraDemo{
gaia::common::gaia_type_t kCameraImageType=1;
}
extern "C" void initialize_rules()
{
    rule_binding_t  ObjectClassify_handler("cameraDemo","object_class",cameraDemo::ObjectClassify_handler);
    rule_binding_t  ImageDelete_handler("cameraDemo","image_delete",cameraDemo::ImageDelete_handler);
    rule_binding_t  ImageCreate_handler("cameraDemo","image_create",cameraDemo::ImageCreate_handler);
    subscribe_table_rule(CameraDemo::kCameraImageType=1, event_type_t::row_insert,ImageCreate_handler);
    subscribe_table_rule(CameraDemo::kCameraImageType=1, event_type_t::row_delete,ImageDelete_handler);
    subscribe_table_rule(CameraDemo::kObjectType=2, event_type_t::row_insert,ObjectClassify_handler);
}