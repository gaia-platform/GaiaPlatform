/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Automatically generated by the Gaia Data Classes code generator.
// Do not modify.

#include <iterator>

#ifndef GAIA_GENERATED_hello_H_
#define GAIA_GENERATED_hello_H_

#include "gaia/direct_access/edc_object.hpp"
#include "hello_generated.h"
#include "gaia/direct_access/edc_iterators.hpp"

namespace gaia {
namespace hello {

// The initial size of the flatbuffer builder buffer.
constexpr int c_flatbuffer_builder_size = 128;

// Constants contained in the greetings object.
constexpr uint32_t c_gaia_type_greetings = 8u;
constexpr int c_greetings_parent_names = 0;
constexpr int c_greetings_next_names = 1;

// Constants contained in the names object.
constexpr uint32_t c_gaia_type_names = 7u;
constexpr int c_names_first_greetings = 0;

struct greetings_t;
struct names_t;

typedef gaia::direct_access::edc_writer_t<c_gaia_type_names, names_t, internal::names, internal::namesT> names_writer;
struct names_t : public gaia::direct_access::edc_object_t<c_gaia_type_names, names_t, internal::names, internal::namesT> {
    typedef gaia::direct_access::reference_chain_container_t<greetings_t> greetings_list_t;
    names_t() : edc_object_t("names_t") {}
    const char* name() const {return GET_STR(name);}
    static gaia::common::gaia_id_t insert_row(const char* name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(internal::CreatenamesDirect(b, name));
        return edc_object_t::insert_row(b);
    }
    static gaia::direct_access::edc_container_t<c_gaia_type_names, names_t>& list() {
        static gaia::direct_access::edc_container_t<c_gaia_type_names, names_t> list;
        return list;
    }

    greetings_t greetings() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<names_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<names_t, const char*> name;
        static gaia::direct_access::expression_t<names_t, greetings_t> greetings;
    };
    using expr = expr_<void>;

private:
    friend class edc_object_t<c_gaia_type_names, names_t, internal::names, internal::namesT>;
    explicit names_t(gaia::common::gaia_id_t id) : edc_object_t(id, "names_t") {}
};

template<class unused_t> gaia::direct_access::expression_t<names_t, gaia::common::gaia_id_t> names_t::expr_<unused_t>::gaia_id{&names_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<names_t, const char*> names_t::expr_<unused_t>::name{&names_t::name};
template<class unused_t> gaia::direct_access::expression_t<names_t, greetings_t> names_t::expr_<unused_t>::greetings{&names_t::greetings};

namespace names_expr {
    static auto& name = names_t::expr::name;
    static auto& greetings = names_t::expr::greetings;
};

typedef gaia::direct_access::edc_writer_t<c_gaia_type_greetings, greetings_t, internal::greetings, internal::greetingsT> greetings_writer;
struct greetings_t : public gaia::direct_access::edc_object_t<c_gaia_type_greetings, greetings_t, internal::greetings, internal::greetingsT> {
    greetings_t() : edc_object_t("greetings_t") {}
    const char* greeting() const {return GET_STR(greeting);}
    static gaia::common::gaia_id_t insert_row(const char* greeting) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(internal::CreategreetingsDirect(b, greeting));
        return edc_object_t::insert_row(b);
    }
    static gaia::direct_access::edc_container_t<c_gaia_type_greetings, greetings_t>& list() {
        static gaia::direct_access::edc_container_t<c_gaia_type_greetings, greetings_t> list;
        return list;
    }
    names_t names() const {
        return names_t::get(this->references()[c_greetings_parent_names]);
    }
    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<greetings_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<greetings_t, const char*> greeting;
        static gaia::direct_access::expression_t<greetings_t, names_t> names;
    };
    using expr = expr_<void>;

private:
    friend class edc_object_t<c_gaia_type_greetings, greetings_t, internal::greetings, internal::greetingsT>;
    explicit greetings_t(gaia::common::gaia_id_t id) : edc_object_t(id, "greetings_t") {}
};

template<class unused_t> gaia::direct_access::expression_t<greetings_t, gaia::common::gaia_id_t> greetings_t::expr_<unused_t>::gaia_id{&greetings_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<greetings_t, const char*> greetings_t::expr_<unused_t>::greeting{&greetings_t::greeting};
template<class unused_t> gaia::direct_access::expression_t<greetings_t, names_t> greetings_t::expr_<unused_t>::names{&greetings_t::names};

namespace greetings_expr {
    static auto& greeting = greetings_t::expr::greeting;
    static auto& names = greetings_t::expr::names;
};

}  // namespace hello
}  // namespace gaia

#endif  // GAIA_GENERATED_hello_H_

