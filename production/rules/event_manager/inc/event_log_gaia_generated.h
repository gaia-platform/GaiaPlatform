// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_GAIA_EVENTLOG_GAIA_RULES_H_
#define FLATBUFFERS_GENERATED_GAIA_EVENTLOG_GAIA_RULES_H_

#include "event_log_generated.h"
#include "gaia_object.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia {
namespace rules {

static const gaia::common::gaia_type_t kEvent_logType = 1;
}  // namespace rules
}  // namespace gaia

namespace gaia {
namespace rules {

struct Event_log : public gaia_object_t<gaia::rules::kEvent_logType,Event_log,event_log,event_logT>{
Event_log() = default;
uint64_t id () const { return GET_CURRENT(id);}
uint64_t id_original () const { return GET_ORIGINAL(id);}
void set_id(uint64_t val)
{
SET(id, val);
}
uint64_t gaia_type () const { return GET_CURRENT(gaia_type);}
uint64_t gaia_type_original () const { return GET_ORIGINAL(gaia_type);}
void set_gaia_type(uint64_t val)
{
SET(gaia_type, val);
}
uint32_t event_type () const { return GET_CURRENT(event_type);}
uint32_t event_type_original () const { return GET_ORIGINAL(event_type);}
void set_event_type(uint32_t val)
{
SET(event_type, val);
}
uint8_t event_mode () const { return GET_CURRENT(event_mode);}
uint8_t event_mode_original () const { return GET_ORIGINAL(event_mode);}
void set_event_mode(uint8_t val)
{
SET(event_mode, val);
}
uint64_t timestamp () const { return GET_CURRENT(timestamp);}
uint64_t timestamp_original () const { return GET_ORIGINAL(timestamp);}
void set_timestamp(uint64_t val)
{
SET(timestamp, val);
}
bool rules_fired () const { return GET_CURRENT(rules_fired);}
bool rules_fired_original () const { return GET_ORIGINAL(rules_fired);}
void set_rules_fired(bool val)
{
SET(rules_fired, val);
}
void update_row(){
gaia_object_t::update_row();
}
void insert_row(){
gaia_object_t::insert_row();
}
void delete_row(){
gaia_object_t::delete_row();
}

static void insert_row(uint64_t id = 0,
    uint64_t gaia_type = 0,
    uint32_t event_type = 0,
    uint8_t event_mode = 0,
    uint64_t timestamp = 0,
    bool rules_fired = false)
{
    flatbuffers::FlatBufferBuilder fbb;
    auto fb = Createevent_log(fbb, id, gaia_type, event_type, event_mode, timestamp, rules_fired);
    fbb.Finish(fb);
    gaia_object_t::insert_row(fbb);
}

static void begin_transaction(){
gaia_object_t::begin_transaction();
}
static void commit_transaction(){
gaia_object_t::commit_transaction();
}
static void rollback_transaction(){
gaia_object_t::rollback_transaction();
}
private:
friend struct gaia_object_t<gaia::rules::kEvent_logType,Event_log,event_log,event_logT>;
Event_log(gaia_id_t id) : gaia_object_t(id) {}
};

}  // namespace rules
}  // namespace gaia

#endif  // FLATBUFFERS_GENERATED_GAIA_EVENTLOG_GAIA_RULES_H_
