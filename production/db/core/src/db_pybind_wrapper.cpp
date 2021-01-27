/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iomanip>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/db/gaia_ptr.hpp"

using namespace std;
using namespace gaia::db;
using namespace pybind11;

void print_payload(ostream& o, const size_t size, const char* payload)
{
    if (size == 0)
    {
        return;
    }

    o << " Payload: ";
    for (size_t i = 0; i < size; i++)
    {
        char character = payload[i];
        if (isprint(character))
        {
            o << character;
        }
        else
        {
            o << ".";
        }
    }
}

void print_node(const gaia_ptr& node, const bool indent = false)
{
    cout << endl;

    if (indent)
    {
        cout << "  ";
    }

    if (!node)
    {
        cout << "<null_node>";
        return;
    }

    cout
        << "Node id: " << node.id()
        << ", type: " << node.type()
        << ", payload size: " << node.data_size()
        << ";"
        << endl;

    print_payload(cout, node.data_size(), node.data());

    if (indent)
    {
        return;
    }

    cout << endl;
}

PYBIND11_MODULE(gaia_db_pybind, m)
{
    m.doc() = "Python wrapper over Gaia Database.";

    m.def("begin_session", &gaia::system::initialize, arg("gaia_config_file") = nullptr, arg("logger_config_file") = nullptr);
    m.def("end_session", &end_session);
    m.def("begin_transaction", &begin_transaction);
    m.def("commit_transaction", &commit_transaction);
    m.def("rollback_transaction", &rollback_transaction);

    m.def("print_node", &print_node);

    register_exception<gaia::db::session_exists>(m, "session_exists");
    register_exception<gaia::db::no_active_session>(m, "no_session_active");
    register_exception<gaia::db::transaction_in_progress>(m, "transaction_in_progress");
    register_exception<gaia::db::no_open_transaction>(m, "transaction_not_open");
    register_exception<gaia::db::transaction_update_conflict>(m, "transaction_update_conflict");
    register_exception<gaia::db::transaction_object_limit_exceeded>(m, "transaction_object_limit_exceeded");
    register_exception<gaia::db::duplicate_id>(m, "duplicate_id");
    register_exception<gaia::db::oom>(m, "oom");
    register_exception<gaia::db::invalid_node_id>(m, "invalid_node_id");
    register_exception<gaia::db::invalid_id_value>(m, "invalid_id_value");
    register_exception<gaia::db::node_not_disconnected>(m, "node_not_disconnected");
    register_exception<gaia::db::payload_size_too_large>(m, "payload_size_too_large");
    register_exception<gaia::db::invalid_type>(m, "invalid_type");

    class_<gaia_ptr>(m, "gaia_ptr")
        .def_static(
            "create",
            static_cast<gaia_ptr (*)(gaia_type_t, size_t, const void*)>(&gaia_ptr::create))
        .def_static(
            "create",
            static_cast<gaia_ptr (*)(gaia_id_t, gaia_type_t, size_t, const void*)>(&gaia_ptr::create))
        .def_static(
            "create",
            static_cast<gaia_ptr (*)(gaia_id_t, gaia_type_t, size_t, size_t, const void*)>(&gaia_ptr::create))
        .def_static("open", &gaia_ptr::open)
        .def_static("find_first", &gaia_ptr::find_first)
        .def_static("remove", &gaia_ptr::remove)
        .def("is_null", &gaia_ptr::is_null)
        .def("id", &gaia_ptr::id)
        .def("type", &gaia_ptr::type)
        .def("data", &gaia_ptr::data, return_value_policy::reference)
        .def("data_size", &gaia_ptr::data_size)
        .def("references", &gaia_ptr::references, return_value_policy::reference)
        .def("num_references", &gaia_ptr::num_references)
        .def("find_next", static_cast<gaia_ptr (gaia_ptr::*)()>(&gaia_ptr::find_next));
}
