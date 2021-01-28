/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

#include "cow_se.h"

using namespace std;
using namespace gaia_se;
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
            o << "\\x" <<
                setw(2) << setfill('0') << hex <<
                short(payload[i]) << dec;
        }
    }
}

void print_node(const gaia_ptr<gaia_se_node>& node, const bool indent = false);

void print_edge(const gaia_ptr<gaia_se_edge>& edge, const bool indent = false)
{
    cout << endl;

    if (indent)
    {
        cout << "  ";
    }

    if (!edge)
    {
        cout << "<null_edge>";
        return;
    }

    cout
        << "Edge id:"
        << edge->id << ", type:"
        << edge->type;

    print_payload (cout, edge->payload_size, edge->payload);

    if (!indent)
    {
        print_node (edge->node_first, true);
        print_node (edge->node_second, true);
        cout << endl;
    }
    else
    {
    cout
        << " first: " << edge->first
        << " second: " << edge->second;
    }
}

void print_node(const gaia_ptr<gaia_se_node>& node, const bool indent)
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
        << node->id << ", type:"
        << node->type;

    print_payload (cout, node->payload_size, node->payload);

    if (indent || (!node->next_edge_first && ! node->next_edge_second))
    {
        return;
    }

    for (auto edge = node->next_edge_first; edge; edge = edge->next_edge_first)
    {
        print_edge(edge, true);
    }

    for (auto edge = node->next_edge_second; edge; edge = edge->next_edge_second)
    {
        print_edge(edge, true);
    }

    cout << endl;
}

pybind11::bytes get_bytes(const pybind11::object& o)
{
    char* ptr = nullptr;
    size_t size = 0;

    if (PyString_Check(o.ptr()) || PyBytes_Check(o.ptr()))
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

gaia_ptr<gaia_se_node> create_node (
    gaia_id_t id,
    gaia_type_t type,
    const pybind11::object& payload)
{
    pybind11::bytes bytes_payload = get_bytes(payload);
    return gaia_se_node::create(id, type, PyBytes_Size(bytes_payload.ptr()), PyBytes_AsString(bytes_payload.ptr()));
}

gaia_ptr<gaia_se_edge> create_edge(
    gaia_id_t id,
    gaia_type_t type,
    gaia_id_t first,
    gaia_id_t second,
    const pybind11::object& payload)
{
    pybind11::bytes bytes_payload = get_bytes(payload);
    return gaia_se_edge::create(id, type, first, second, PyBytes_Size(bytes_payload.ptr()), PyBytes_AsString(bytes_payload.ptr()));
}

void update_node(gaia_ptr<gaia_se_node> node, const pybind11::object& payload)
{
    if (node.is_null())
    {
        throw invalid_argument("update_node() was called with a null node parameter!");
    }

    pybind11::bytes bytes_payload = get_bytes(payload);
    node.update_payload(PyBytes_Size(bytes_payload.ptr()), PyBytes_AsString(bytes_payload.ptr()));
}

void update_edge(gaia_ptr<gaia_se_edge> edge, const pybind11::object& payload)
{
    if (edge.is_null())
    {
        throw invalid_argument("update_edge() was called with a null edge parameter!");
    }

    pybind11::bytes bytes_payload = get_bytes(payload);
    edge.update_payload(PyBytes_Size(bytes_payload.ptr()), PyBytes_AsString(bytes_payload.ptr()));
}

pybind11::bytes get_node_payload(gaia_ptr<gaia_se_node> node)
{
    if (node.is_null())
    {
        throw invalid_argument("get_node_payload() was called with a null node parameter!");
    }

    return pybind11::bytes(node.get()->payload, node.get()->payload_size);
}

pybind11::bytes get_edge_payload(gaia_ptr<gaia_se_edge> edge)
{
    if (edge.is_null())
    {
        throw invalid_argument("get_edge_payload() was called with a null edge parameter!");
    }

    return pybind11::bytes(edge.get()->payload, edge.get()->payload_size);
}

PYBIND11_MODULE(cow_se, m)
{
    m.doc() = "Python wrapper over Gaia COW storage engine prototype.";

    m.def("begin_transaction", &begin_transaction);
    m.def("commit_transaction", &commit_transaction);
    m.def("rollback_transaction", &rollback_transaction);

    m.def("print_node", &print_node);
    m.def("print_edge", &print_edge);

    m.def("create_node", &create_node);
    m.def("create_edge", &create_edge);

    m.def("update_node", &update_node);
    m.def("update_edge", &update_edge);

    m.def("get_node_payload", &get_node_payload);
    m.def("get_edge_payload", &get_edge_payload);

    register_exception<gaia_se::tx_not_open>(m, "tx_not_open");
    register_exception<gaia_se::tx_update_conflict>(m, "tx_update_conflict");
    register_exception<gaia_se::duplicate_id>(m, "duplicate_id");
    register_exception<gaia_se::oom>(m, "oom");
    register_exception<gaia_se::dependent_edges_exist>(m, "dependent_edges_exist");
    register_exception<gaia_se::invalid_node_id>(m, "invalid_node_id");

    class_<gaia_ptr<gaia_se_node>>(m, "gaia_se_node_ptr")
        .def_static("find_first", &gaia_ptr<gaia_se_node>::find_first)
        .def_static("remove", &gaia_ptr<gaia_se_node>::remove)
        .def("find_next", (gaia_ptr<gaia_se_node> (gaia_ptr<gaia_se_node>::*)()) &gaia_ptr<gaia_se_node>::find_next)
        .def("is_null", &gaia_ptr<gaia_se_node>::is_null)
        .def("get", &gaia_ptr<gaia_se_node>::get, return_value_policy::reference)
    ;
    class_<gaia_ptr<gaia_se_edge>>(m, "gaia_se_edge_ptr")
        .def_static("find_first", &gaia_ptr<gaia_se_edge>::find_first)
        .def_static("remove", &gaia_ptr<gaia_se_edge>::remove)
        .def("find_next", (gaia_ptr<gaia_se_edge> (gaia_ptr<gaia_se_edge>::*)()) &gaia_ptr<gaia_se_edge>::find_next)
        .def("is_null", &gaia_ptr<gaia_se_edge>::is_null)
        .def("get", &gaia_ptr<gaia_se_edge>::get, return_value_policy::reference)
    ;

    class_<gaia_mem_base>(m, "gaia_mem_base")
        .def(init())
        .def_static("init", &gaia_mem_base::init)
    ;
    class_<gaia_se_node>(m, "gaia_se_node")
        .def_static("open", &gaia_se_node::open)
        .def_readonly("id", &gaia_se_node::id)
        .def_readonly("type", &gaia_se_node::type)
        .def_readonly("next_edge_first", &gaia_se_node::next_edge_first)
        .def_readonly("next_edge_second", &gaia_se_node::next_edge_second)
    ;
    class_<gaia_se_edge>(m, "gaia_se_edge")
        .def_static("open", &gaia_se_edge::open)
        .def_readonly("id", &gaia_se_edge::id)
        .def_readonly("type", &gaia_se_edge::type)
        .def_readonly("first", &gaia_se_edge::first)
        .def_readonly("second", &gaia_se_edge::second)
        .def_readonly("node_first", &gaia_se_edge::node_first)
        .def_readonly("node_second", &gaia_se_edge::node_second)
        .def_readonly("next_edge_first", &gaia_se_edge::next_edge_first)
        .def_readonly("next_edge_second", &gaia_se_edge::next_edge_second)
    ;
};
