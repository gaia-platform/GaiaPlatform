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
gaia::common::gaia_type_t kObjectType;
}
namespace CameraDemo{
gaia::common::gaia_type_t kCameraImageType;
}
extern "C" void initialize_rules()
{
    cerr << "qqq" << endl;
    rule_binding_t  ObjectClassify_handler("cameraDemo","object_class",cameraDemo::ObjectClassify_handler);
    cerr << "1" << endl;
    rule_binding_t  ImageDelete_handler("cameraDemo","image_delete",cameraDemo::ImageDelete_handler);
    rule_binding_t  ImageCreate_handler("cameraDemo","image_create",cameraDemo::ImageCreate_handler);
    cerr << "2" << endl;
    subscribe_table_rule(CameraDemo::kCameraImageType, event_type_t::row_insert,ImageCreate_handler);
    cerr << "3" << endl;
    subscribe_table_rule(CameraDemo::kCameraImageType, event_type_t::row_delete,ImageDelete_handler);
    subscribe_table_rule(CameraDemo::kObjectType, event_type_t::row_insert,ObjectClassify_handler);
    
}
