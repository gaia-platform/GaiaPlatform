#include "gaia_school.h"
#include "school_sim.h"
#include <atomic>

#pragma once

using namespace std;
using namespace gaia::school;
using namespace gaia::common;
using namespace gaia::direct_access;

static std::atomic<uint64_t> g_face_signature{10};

int64_t get_face_signature() {
  return g_face_signature++;
}

gaia_id_t create_staff(const std::string& firstName, const std::string& lastName,
                int64_t birthDate, int64_t hireDate) {
  auto person = person_t::get(person_t::insert_row(firstName.c_str(), lastName.c_str(), birthDate, g_face_signature++));
  auto staff = staff_t::insert_row(hireDate);
  person.StaffAsPerson_staff_list().insert(staff);
  return staff;
}

gaia_id_t create_student(const std::string& firstName, const std::string& lastName, int64_t birthDate, const std::string& number) {
  auto person = person_t::get(person_t::insert_row(firstName.c_str(), lastName.c_str(), birthDate, g_face_signature++));
  auto student = student_t::insert_row(number.c_str());
  person.StudentAsPerson_student_list().insert(student);
  return student;
}

gaia_id_t create_parent(const std::string& firstName, const std::string& lastName, int64_t birthDate, bool MotherOrFather) {
  auto person = person_t::get(person_t::insert_row(firstName.c_str(), lastName.c_str(), birthDate, g_face_signature++));
  auto parent = parent_t::insert_row(MotherOrFather);
  person.ParentAsPerson_parent_list().insert(parent);
  return parent;
}

gaia_id_t create_facescan(building_t& building) {
  auto fsl = FaceScanLog_t::insert_row(school_myface(), school_now());
  building.Building_FaceScanLog_list().insert(fsl);

  return fsl;
}

gaia_id_t create_event(const std::string& name, int64_t start_time, int64_t end_time, room_t& room, staff_t& teacher) {
  auto evt = event_t::insert_row(name.c_str(), 1, start_time, end_time, 0, 0);
  room.EventRoom_event_list().insert(evt);
  teacher.Teacher_event_list().insert(evt);

  return evt;
}

gaia_id_t register_student(student_t& student, event_t& event) {
  auto registration = registration_t::insert_row(school_now());
  student.Student_registration_list().insert(registration);
  event.Event_registration_list().insert(registration);
  return registration;
}
