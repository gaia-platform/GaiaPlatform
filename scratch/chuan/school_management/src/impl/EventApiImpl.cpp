/**
 * Gaia School
 * School management system REST API
 *
 * The version of the OpenAPI document: 1.0.0
 *
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * (https://openapi-generator.tech). https://openapi-generator.tech Do not edit
 * the class manually.
 */

#include "EventApiImpl.h"

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/exception.hpp"
#include "gaia_school.h"

#include "scope_guard.hpp"

namespace org {
namespace openapitools {
namespace server {
namespace api {

using namespace org::openapitools::server::model;

EventApiImpl::EventApiImpl(std::shared_ptr<Pistache::Rest::Router> rtr)
    : EventApi(rtr) {}

void EventApiImpl::add_event(const Event &event,
                             Pistache::Http::ResponseWriter &response) {
  response.headers().add<Pistache::Http::Header::ContentType>(
      MIME(Application, Json));
  nlohmann::json j;

  if (!event.nameIsSet() || event.getName().empty() || !event.staffIdIsSet() ||
      event.getStaffId() == gaia::common::c_invalid_gaia_id ||
      !event.roomIdIsSet() ||
      event.getRoomId() == gaia::common::c_invalid_gaia_id) {
    j["what"] = "Incomplete event information.";
    response.send(Pistache::Http::Code::Bad_Request, j.dump());
    return;
  }

  gaia::db::begin_session();
  auto db_session_cleanup =
      sg::make_scope_guard([]() { gaia::db::end_session(); });

  try {
    gaia::direct_access::auto_transaction_t txn;
    auto staff = gaia::school::staff_t::get(event.getStaffId());
    auto room = gaia::school::room_t::get(event.getRoomId());
    auto eventId = gaia::school::event_t::insert_row(
        event.getName().c_str(), event.getStartTime(), event.getEndTime());
    room.venue_event_list().insert(eventId);
    staff.teacher_event_list().insert(eventId);
    txn.commit();
    j["eventId"] = eventId;
  } catch (const gaia::common::gaia_exception &e) {
    j["what"] = e.what();
    response.send(Pistache::Http::Code::Bad_Request, j.dump());
    return;
  }

  response.send(Pistache::Http::Code::Ok, j.dump());
}
void EventApiImpl::add_event_participant(
    const int64_t &eventId, const std::vector<int64_t> &requestBody,
    Pistache::Http::ResponseWriter &response) {
  gaia::db::begin_session();
  auto db_session_cleanup =
      sg::make_scope_guard([]() { gaia::db::end_session(); });
  response.headers().add<Pistache::Http::Header::ContentType>(
      MIME(Application, Json));
  nlohmann::json j;

  try {
    gaia::direct_access::auto_transaction_t txn;
    auto regId = gaia::school::registration_t::insert_row(
        std::chrono::system_clock::now().time_since_epoch().count());
    auto event = gaia::school::event_t::get(eventId);
    event.event_registration_list().insert(regId);
    for (auto studentId : requestBody) {
      auto student = gaia::school::student_t::get(studentId);
      student.student_registration_list().insert(regId);
    }
    txn.commit();
    j["registrationId"] = regId;
  } catch (const gaia::common::gaia_exception &e) {
    j["what"] = e.what();
    response.send(Pistache::Http::Code::Bad_Request, j.dump());
    return;
  }

  response.send(Pistache::Http::Code::Ok, j.dump());
}
void EventApiImpl::delete_event(const int64_t &eventId,
                                Pistache::Http::ResponseWriter &response) {
  response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}
void EventApiImpl::drop_event_participant(
    const int64_t &eventId, const int64_t &studentId,
    Pistache::Http::ResponseWriter &response) {
  response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}
void EventApiImpl::get_event_by_id(const int64_t &eventId,
                                   Pistache::Http::ResponseWriter &response) {
  response.headers().add<Pistache::Http::Header::ContentType>(
      MIME(Application, Json));
  nlohmann::json j;

  gaia::db::begin_session();
  auto db_session_cleanup =
      sg::make_scope_guard([]() { gaia::db::end_session(); });

  try {
    gaia::direct_access::auto_transaction_t txn;
    auto event = gaia::school::event_t::get(eventId);

    Event e;
    e.setId(event.gaia_id());
    e.setName(event.name());
    e.setStartTime(event.start_time());
    e.setEndTime(event.end_time());
    e.setRoomId(event.venue_room().gaia_id());
    e.setStaffId(event.teacher_staff().gaia_id());
    txn.commit();
    j = e;
  } catch (const gaia::common::gaia_exception &e) {
    j["what"] = e.what();
    response.send(Pistache::Http::Code::Bad_Request, j.dump());
    return;
  }

  response.send(Pistache::Http::Code::Ok, j.dump());
}
void EventApiImpl::list_event_participant(
    const int64_t &eventId, Pistache::Http::ResponseWriter &response) {
  gaia::db::begin_session();
  auto db_session_cleanup =
      sg::make_scope_guard([]() { gaia::db::end_session(); });
  response.headers().add<Pistache::Http::Header::ContentType>(
      MIME(Application, Json));
  nlohmann::json j;

  try {
    gaia::direct_access::auto_transaction_t txn;
    for (auto reg :
         gaia::school::event_t::get(eventId).event_registration_list()) {
      j.push_back(reg.student_student().gaia_id());
    }
    txn.commit();
  } catch (const gaia::common::gaia_exception &e) {
    j["what"] = e.what();
    response.send(Pistache::Http::Code::Bad_Request, j.dump());
    return;
  }

  response.send(Pistache::Http::Code::Ok, j.dump());
}
void EventApiImpl::list_events(Pistache::Http::ResponseWriter &response) {
  gaia::db::begin_session();
  auto db_session_cleanup =
      sg::make_scope_guard([]() { gaia::db::end_session(); });
  response.headers().add<Pistache::Http::Header::ContentType>(
      MIME(Application, Json));
  nlohmann::json j;

  try {
    gaia::direct_access::auto_transaction_t txn;
    for (auto event : gaia::school::event_t::list()) {
      Event e;
      e.setId(event.gaia_id());
      e.setName(event.name());
      e.setStartTime(event.start_time());
      e.setEndTime(event.end_time());
      e.setRoomId(event.venue_room().gaia_id());
      e.setStaffId(event.teacher_staff().gaia_id());
      j.push_back(e);
    }
    txn.commit();
  } catch (const gaia::common::gaia_exception &e) {
    j.clear();
    j["what"] = e.what();
    response.send(Pistache::Http::Code::Bad_Request, j.dump());
    return;
  }

  response.send(Pistache::Http::Code::Ok, j.dump());
}
void EventApiImpl::update_event(const Event &event,
                                Pistache::Http::ResponseWriter &response) {
  response.send(Pistache::Http::Code::Ok, "Do some magic\n");
}

} // namespace api
} // namespace server
} // namespace openapitools
} // namespace org
