/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "file.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <retail_assert.hpp>

using namespace std;
using namespace gaia::common;

file_open_error::file_open_error(const char* filename)
{
    stringstream string_stream;
    string_stream << "An error has occurred while attempting to read file '" << filename << "'.";
    m_message = string_stream.str();
}

file_loader_t::file_loader_t()
{
    initialize();
}

file_loader_t::~file_loader_t()
{
    clear();
}

void file_loader_t::initialize()
{
    m_filename = "";
    m_data = nullptr;
    m_data_length = 0;
}

void file_loader_t::clear()
{
    if (m_data != nullptr)
    {
        delete[] m_data;
    }

    initialize();
}

size_t file_loader_t::load_file_data(const string& filename)
{
    retail_assert(!filename.empty(), "load_file_data() was called with an invalid filename argument.");

    // If we had already loaded data from another file,
    // clear our state first.
    if (m_data != nullptr)
    {
        clear();
    }

    m_filename = filename;

    ifstream file;
    file.open(m_filename, ios::binary | ios::in);

    if (!file.good())
    {
        file.close();
        throw file_open_error(m_filename.c_str());
    }

    // Get file length.
    file.seekg(0, ios::end);
    m_data_length = file.tellg();

    // Allocate buffer and read file data.
    if (m_data_length > 0)
    {
        m_data = new uint8_t[m_data_length];
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(m_data), m_data_length);
    }

    file.close();

    return m_data_length;
}

uint8_t* file_loader_t::get_data()
{
    return m_data;
}

size_t file_loader_t::get_data_length()
{
    return m_data_length;
}
