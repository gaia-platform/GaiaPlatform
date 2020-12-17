/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/*
 * File:   Log.h
 * Author: Alberto Lepe <dev@alepe.com>
 *
 * Created on December 1, 2015, 6:00 PM
 */

#pragma once

#include <iostream>

enum log_level_t
{
    debug,
    info,
    warn,
    error
};

struct log_conf_t
{
    bool headers = false;
    log_level_t level = warn;
};

extern log_conf_t g_log_conf;

class logger_t
{
public:
    logger_t() = default;
    explicit logger_t(log_level_t type)
    {
        m_msglevel = type;
        if (g_log_conf.headers)
        {
            operator<<("[" + get_label(type) + "]");
        }
    }

    ~logger_t()
    {
        if (m_opened)
        {
            std::cout << std::endl << std::flush;
        }
        m_opened = false;
    }

    template <class T>
    logger_t& operator<<(const T& msg)
    {
        if (m_msglevel >= g_log_conf.level)
        {
            std::cout << msg;
            m_opened = true;
        }
        return *this;
    }

private:
    bool m_opened = false;
    log_level_t m_msglevel = debug;

    inline std::string get_label(log_level_t type)
    {
        std::string label;
        switch (type)
        {
        case debug:
            label = "debug";
            break;
        case info:
            label = "info ";
            break;
        case warn:
            label = "warn ";
            break;
        case error:
            label = "error";
            break;
        }
        return label;
    }
};

class rule_logger_t : public logger_t
{
public:
    explicit rule_logger_t(log_level_t type, uint32_t rule_id)
        : logger_t(type)
    {
        if (g_log_conf.headers)
        {
            operator<<("[rule-" + std::to_string(rule_id) + "] ");
        }
    }
};
