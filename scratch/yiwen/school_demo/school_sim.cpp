#include <iomanip>
#include <ctime>
#include <sstream>
#include "school_sim.h"

gaia::common::gaia_id_t c_sim_frame_id;
constexpr int64_t c_start_sim = 775731600; // 1994 Aug 1 9am

constexpr int64_t c_voldemort_face = 1;
constexpr const char* c_voldemort_name = "Lord Voldemort";

void start_sim() {
  auto_transaction_t txn;
  c_sim_frame_id = gaia::school::sim_t::insert_row(c_start_sim, c_start_sim + 24 * 60 * 60, c_voldemort_face, c_voldemort_name);
  txn.commit();
}

void play_as_stranger() {
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  auto w = frame.writer();
  w.MyFace = c_voldemort_face;
  w.MyName = c_voldemort_name;
  w.update_row();
}

void play_as_stranger(int64_t face_signature, const char* name) {
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  auto w = frame.writer();
  w.MyFace = face_signature;
  w.MyName = name;
  w.update_row();
}

void play_as_person(gaia::school::person_t& person) {
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  auto w = frame.writer();
  std::ostringstream name;
  name << person.FirstName() << " " << person.LastName();
  w.MyName = name.str().c_str();
  w.MyFace = person.FaceSignature();
  w.update_row();
}

void advance_hour() {
  auto_transaction_t txn;
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  auto w = frame.writer();

  w.now = frame.now() + 60 * 60;
  w.update_row();
  txn.commit();
}

void advance_day() {
  auto_transaction_t txn;
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  auto w = frame.writer();

  w.now = frame.NextDay();
  w.update_row();
  txn.commit();
}

void time_travel(int64_t time) {
  auto_transaction_t txn;
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  auto w = frame.writer();

  w.now = time;
  w.update_row();
  txn.commit();
}

void msg_number(const std::string& number, const std::string& msg) {
  gaia::school::msg_t::insert_row(number.c_str(), msg.c_str());
}

int64_t school_now() {
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  return frame.now();
}

std::string school_whoami() {
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  return frame.MyName();
}

int64_t school_myface() {
  auto frame = gaia::school::sim_t::get(c_sim_frame_id);
  return frame.MyFace();
}

void print_time (int64_t time, const char* fmt) {
  std::time_t tm = static_cast<time_t>(time);
  auto info = std::gmtime(&tm);
  std::cout << std::put_time(info, fmt);
}
