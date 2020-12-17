#include <atomic>
#include <iostream>
#include <string>

#include "gaia_school.h"

#pragma once

// Today's date and time
using namespace gaia::db;
using namespace gaia::direct_access;

void start_sim();
void advance_hour();
void advance_day();
void msg_number(const std::string& number, const std::string& msg);
int64_t school_now();
std::string school_whoami();
int64_t school_myface();
void time_travel(int64_t);
void print_time(int64_t time, const char* fmt);
void play_as_person(gaia::school::person_t& person);
void play_as_stranger(int64_t face_signature, const char* name); 
void play_as_stranger();

