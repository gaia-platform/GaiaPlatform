/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Automatically generated by Gaia Data Classes code generator, do not modify.

#include <iterator>

#ifndef GAIA_GENERATED_cameraDemo_H_
#define GAIA_GENERATED_cameraDemo_H_

#include "gaia/direct_access/gaia_object.hpp"
#include "cameraDemo_generated.h"
#include "gaia/direct_access/gaia_iterators.hpp"

using namespace std;
using namespace gaia::direct_access;

namespace gaia {
namespace cameraDemo {

// The initial size of the flatbuffer builder buffer.
constexpr int c_flatbuffer_builder_size = 128;

// Constants contained in the emergency_stop object.
constexpr int c_num_emergency_stop_ptrs = 0;

// Constants contained in the object object.
constexpr int c_num_object_ptrs = 0;

// Constants contained in the camera_image object.
constexpr int c_num_camera_image_ptrs = 0;

struct emergency_stop_t;
struct object_t;
struct camera_image_t;

typedef gaia_writer_t<42llu,camera_image_t,camera_image,camera_imageT,c_num_camera_image_ptrs> camera_image_writer;
struct camera_image_t : public gaia_object_t<42llu,camera_image_t,camera_image,camera_imageT,c_num_camera_image_ptrs> {
    const char* file_name() const {return GET_STR(file_name);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* file_name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Createcamera_imageDirect(b, file_name));
        return gaia_object_t::insert_row(b);
    }
    static gaia_container_t<42llu, camera_image_t>& list() {
        static gaia_container_t<42llu, camera_image_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<42llu, camera_image_t, camera_image, camera_imageT, c_num_camera_image_ptrs>;
    camera_image_t(gaia_id_t id) : gaia_object_t(id, "camera_image_t") {
    }
};

typedef gaia_writer_t<44llu,object_t,object,objectT,c_num_object_ptrs> object_writer;
struct object_t : public gaia_object_t<44llu,object_t,object,objectT,c_num_object_ptrs> {
    const char* object_class() const {return GET_STR(object_class);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* object_class) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(CreateobjectDirect(b, object_class));
        return gaia_object_t::insert_row(b);
    }
    static gaia_container_t<44llu, object_t>& list() {
        static gaia_container_t<44llu, object_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<44llu, object_t, object, objectT, c_num_object_ptrs>;
    object_t(gaia_id_t id) : gaia_object_t(id, "object_t") {
    }
};

typedef gaia_writer_t<46llu,emergency_stop_t,emergency_stop,emergency_stopT,c_num_emergency_stop_ptrs> emergency_stop_writer;
struct emergency_stop_t : public gaia_object_t<46llu,emergency_stop_t,emergency_stop,emergency_stopT,c_num_emergency_stop_ptrs> {
    uint64_t until_time_ns() const {return GET(until_time_ns);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(uint64_t until_time_ns) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Createemergency_stop(b, until_time_ns));
        return gaia_object_t::insert_row(b);
    }
    static gaia_container_t<46llu, emergency_stop_t>& list() {
        static gaia_container_t<46llu, emergency_stop_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<46llu, emergency_stop_t, emergency_stop, emergency_stopT, c_num_emergency_stop_ptrs>;
    emergency_stop_t(gaia_id_t id) : gaia_object_t(id, "emergency_stop_t") {
    }
};

}  // namespace cameraDemo
}  // namespace gaia

#endif  // GAIA_GENERATED_cameraDemo_H_

