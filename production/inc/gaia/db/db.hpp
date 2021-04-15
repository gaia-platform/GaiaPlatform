/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

namespace gaia
{

namespace db
{

class session_exists : public common::gaia_exception
{
public:
    session_exists()
    {
        m_message = "Close the current session before creating a new one.";
    }
};

class no_active_session : public common::gaia_exception
{
public:
    no_active_session()
    {
        m_message = "Create a session before performing data access.";
    }
};

class transaction_in_progress : public common::gaia_exception
{
public:
    transaction_in_progress()
    {
        m_message = "Commit or roll back the current transaction before beginning a new transaction.";
    }
};

class no_open_transaction : public common::gaia_exception
{
public:
    no_open_transaction()
    {
        m_message = "Begin a transaction before performing data access.";
    }
};

class transaction_update_conflict : public common::gaia_exception
{
public:
    transaction_update_conflict()
    {
        m_message = "Transaction was aborted due to a serialization error.";
    }
};

class transaction_object_limit_exceeded : public common::gaia_exception
{
public:
    transaction_object_limit_exceeded()
    {
        m_message = "Transaction attempted to update more objects than the system limit.";
    }
};

class duplicate_id : public common::gaia_exception
{
public:
    explicit duplicate_id(common::gaia_id_t id)
    {
        std::stringstream strs;
        strs << "An object with the same ID '" << id << "' already exists.";
        m_message = strs.str();
    }
};

class oom : public common::gaia_exception
{
public:
    oom()
    {
        m_message = "Out of memory.";
    }
};

class invalid_node_id : public common::gaia_exception
{
public:
    explicit invalid_node_id(common::gaia_id_t id)
    {
        std::stringstream strs;
        strs << "Cannot find a node with ID '" << id << "'.";
        m_message = strs.str();
    }
};

class invalid_id_value : public common::gaia_exception
{
public:
    explicit invalid_id_value(common::gaia_id_t id)
    {
        std::stringstream strs;
        strs << "ID value " << id << " is larger than the maximum ID value 2^63.";
        m_message = strs.str();
    }
};

class node_not_disconnected : public common::gaia_exception
{
public:
    node_not_disconnected(common::gaia_id_t id, common::gaia_type_t object_type)
    {
        std::stringstream msg;
        msg
            << "Cannot delete object '" << id << "', type '" << object_type
            << "', because it is still connected to another object.";
        m_message = msg.str();
    }
};

class payload_size_too_large : public common::gaia_exception
{
public:
    payload_size_too_large(size_t total_len, uint16_t max_len)
    {
        std::stringstream msg;
        msg << "Payload size " << total_len << " exceeds maximum payload size limit " << max_len << ".";
        m_message = msg.str();
    }
};

class invalid_type : public common::gaia_exception
{
public:
    explicit invalid_type(common::gaia_type_t type)
    {
        std::stringstream msg;
        msg << "The type '" << type << "' does not exist in the catalog.";
        m_message = msg.str();
    }

    invalid_type(common::gaia_id_t id, common::gaia_type_t type)
    {
        std::stringstream msg;
        msg
            << "Cannot create object with ID '" << id << "' and type '" << type
            << "'. The type does not exist in the catalog.";
        m_message = msg.str();
    }
};

bool is_transaction_active();
void begin_session();
void end_session();
void begin_transaction();
void rollback_transaction();
void commit_transaction();

} // namespace db
} // namespace gaia
