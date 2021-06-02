//
// Created by simone on 5/29/21.
//

//#include "gaia_addr_book.h"

namespace gaia
{
namespace addr_book
{

//B_t A_t::b() const
//{
//    return B_t{this->references()[c_A_first_b]};
//}
//
//void addr_book::A_t::set_b(common::gaia_id_t child_id)
//{
//    insert_child_reference(gaia_id(), child_id, c_A_first_b);
//}
//
//gaia::direct_access::edc_container_t<c_gaia_type_A, A_t>& addr_book::A_t::list()
//{
//    static gaia::direct_access::edc_container_t<c_gaia_type_A, A_t> list;
//    return list;
//}
//gaia::common::gaia_id_t addr_book::A_t::insert_row(const char* str_val, int32_t num_val)
//{
//    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
//    b.Finish(internal::CreateADirect(b, str_val, num_val));
//    return edc_object_t::insert_row(b);
//}
//int32_t addr_book::A_t::num_val() const
//{
//    return GET(num_val);
//}
//const char* addr_book::A_t::str_val() const
//{
//    return GET_STR(str_val);
//}
//A_t addr_book::B_t::a() const
//{
//    return A_t::get(this->references()[c_B_parent_a]);
//}
//gaia::direct_access::edc_container_t<c_gaia_type_B, B_t>& addr_book::B_t::list()
//{
//    static gaia::direct_access::edc_container_t<c_gaia_type_B, B_t> list;
//    return list;
//}
//gaia::common::gaia_id_t addr_book::B_t::insert_row(const char* str_val, int32_t num_val)
//{
//    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
//    b.Finish(internal::CreateBDirect(b, str_val, num_val));
//    return edc_object_t::insert_row(b);
//}
//int32_t addr_book::B_t::num_val() const
//{
//    return GET(num_val);
//}
//const char* addr_book::B_t::str_val() const
//{
//    return GET_STR(str_val);
//}
} // namespace addr_book
} // namespace gaia
