/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

server_config_t::persistence_mode_t server_config_t::persistence_mode()
{
    return m_persistence_mode;
}

const std::string& server_config_t::data_dir()
{
    return m_data_dir;
}

const std::string& server_config_t::instance_name()
{
    return m_instance_name;
}
