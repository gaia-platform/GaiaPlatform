/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iomanip>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia_ptr.hpp"

using namespace std;
using namespace gaia::db;
using namespace pybind11;

void print_payload(ostream& o, const size_t size, const char* payload)
{
    if (size)
    {
        o << " Payload: ";
    }

    for (size_t i = 0; i < size; i++)
    {
        if ('\\' == payload[i])
        {
            o << "\\\\";
        }
        else if (isprint(payload[i]))
        {
            o << payload[i];
        }
        else
        {
            o << "\\x" << setw(2) << setfill('0') << hex << short(payload[i]) << dec;
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
        << "Node id:"
        << node.id() << ", type:"
        << node.type();

    print_payload(cout, node.data_size(), node.data());

    if (indent)
    {
        return;
    }

    cout << endl;
}

pybind11::bytes get_bytes(const pybind11::object& o)
{
    char* ptr = nullptr;
    size_t size = 0;

    if (PyUnicode_Check(o.ptr()))
    {
        ptr = reinterpret_cast<char*>(PyUnicode_DATA(o.ptr())); // NOLINT
        size = PyUnicode_GetLength(o.ptr());
    }
    else if (PyBytes_Check(o.ptr()))
    {
        ptr = PyBytes_AsString(o.ptr());
        size = PyBytes_Size(o.ptr());
    }
    else if (PyByteArray_Check(o.ptr()))
    {
        ptr = PyByteArray_AsString(o.ptr());
        size = PyByteArray_Size(o.ptr());
    }
    else
    {
        throw invalid_argument("Expected a string, bytes, or bytearray argument!");
    }

    if (o.ptr() == Py_None || ptr == nullptr)
    {
        throw invalid_argument("Failed to access argument data!");
    }

    pybind11::bytes b(ptr, size);

    return b;
}

gaia_ptr create_node(
    gaia_id_t id,
    gaia_type_t type,
    const pybind11::object& payload)
{
    pybind11::bytes bytes_payload = get_bytes(payload);
    return gaia_ptr::create(id, type, PyBytes_Size(bytes_payload.ptr()), PyBytes_AsString(bytes_payload.ptr()));
}

void update_node(gaia_ptr node, const pybind11::object& payload)
{
    if (node.is_null())
    {
        throw invalid_argument("update_node() was called with a null node parameter!");
    }

    pybind11::bytes bytes_payload = get_bytes(payload);
    node.update_payload(PyBytes_Size(bytes_payload.ptr()), PyBytes_AsString(bytes_payload.ptr()));
}

pybind11::bytes get_node_payload(gaia_ptr node)
{
    if (node.is_null())
    {
        throw invalid_argument("get_node_payload() was called with a null node parameter!");
    }

    return pybind11::bytes(node.data(), node.data_size());
}

PYBIND11_MODULE(se_client, m)
{
    m.doc() = "Python wrapper over Gaia COW storage engine.";

    m.def("begin_session", &begin_session);
    m.def("end_session", &end_session);
    m.def("begin_transaction", &begin_transaction);
    m.def("commit_transaction", &commit_transaction);
    m.def("rollback_transaction", &rollback_transaction);

    m.def("print_node", &print_node);
    m.def("create_node", &create_node);
    m.def("update_node", &update_node);
    m.def("get_node_payload", &get_node_payload);

    register_exception<gaia::db::session_exists>(m, "session_exists");
    register_exception<gaia::db::no_active_session>(m, "no_session_active");
    register_exception<gaia::db::transaction_in_progress>(m, "transaction_in_progress");
    register_exception<gaia::db::no_open_transaction>(m, "transaction_not_open");
    register_exception<gaia::db::transaction_update_conflict>(m, "transaction_update_conflict");
    register_exception<gaia::db::duplicate_id>(m, "duplicate_id");
    register_exception<gaia::db::oom>(m, "oom");
    register_exception<gaia::db::invalid_node_id>(m, "invalid_node_id");
    register_exception<gaia::db::invalid_id_value>(m, "invalid_id_value");
    register_exception<gaia::db::node_not_disconnected>(m, "node_not_disconnected");

    class_<gaia_ptr>(m, "gaia_ptr")
        .def_static(
            "create",
            static_cast<gaia_ptr (*)(gaia_id_t, gaia_type_t, size_t, const void*)>(&gaia_ptr::create))
        .def_static(
            "create",
            static_cast<gaia_ptr (*)(gaia_id_t, gaia_type_t, size_t, size_t, const void*)>(&gaia_ptr::create))
        .def_static("open", &gaia_ptr::open)
        .def("id", &gaia_ptr::id)
        .def("type", &gaia_ptr::type)
        .def("data", &gaia_ptr::data, return_value_policy::reference)
        .def("data_size", &gaia_ptr::data_size)
        .def("references", &gaia_ptr::references, return_value_policy::reference)
        .def("num_references", &gaia_ptr::num_references)
        .def_static("find_first", &gaia_ptr::find_first)
        .def_static("remove", &gaia_ptr::remove)
        .def("find_next", static_cast<gaia_ptr (gaia_ptr::*)()>(&gaia_ptr::find_next))
        .def("is_null", &gaia_ptr::is_null);
}
