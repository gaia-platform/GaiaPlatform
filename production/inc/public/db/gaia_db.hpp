/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "gaia_common.hpp"
#include "gaia_exception.hpp"

namespace gaia {

namespace db {

using namespace common;

class session_exists : public gaia_exception {
public:
    session_exists() {
        m_message = "Close the current session before creating a new one.";
    }
};

class no_session_active : public gaia_exception {
public:
    no_session_active() {
        m_message = "Create a new session before opening a transaction.";
    }
};

class transaction_in_progress : public gaia_exception {
public:
    transaction_in_progress() {
        m_message = "Commit or roll back the current transaction before beginning a new transaction.";
    }
};

class transaction_not_open : public gaia_exception {
public:
    transaction_not_open() {
        m_message = "Begin a transaction before performing data access.";
    }
};

class transaction_update_conflict : public gaia_exception {
public:
    transaction_update_conflict() {
        m_message = "Transaction was aborted due to a serialization error.";
    }
};

class duplicate_id : public gaia_exception {
public:
    duplicate_id(gaia_id_t id) {
        std::stringstream strs;
        strs << "An object with the same ID (" << id << ") already exists.";
        m_message = strs.str();
    }
};

class oom : public gaia_exception {
public:
    oom() {
        m_message = "Out of memory.";
    }
};

class invalid_node_id : public gaia_exception {
public:
    invalid_node_id(int64_t id) {
        std::stringstream strs;
        strs << "Cannot find a node with ID " << id << ".";
        m_message = strs.str();
    }
};

class invalid_id_value : public gaia_exception {
public:
    invalid_id_value(gaia_id_t id) {
        std::stringstream strs;
        strs << "ID value " << id << " is larger than the maximum ID value 2^63.";
        m_message = strs.str();
    }
};

class node_not_disconnected : public gaia_exception {
public:
    node_not_disconnected(gaia_id_t id, gaia_type_t object_type) {
        stringstream msg;
        msg << "Cannot delete object " << id << ", type " << object_type << " because it is still connected to another object.";
        m_message = msg.str();
    }
};

bool is_transaction_active();
void begin_session();
void end_session();
void begin_transaction();
void rollback_transaction();
void commit_transaction();

const char* const SE_SERVER_NAME = "gaia_se_server";

} // namespace db
} // namespace gaia
