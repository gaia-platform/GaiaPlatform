#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF

#include <ctime>
#include <iomanip>
#include "gaia_school.h"
#include "school_maker.h"
#include "school_sim.h"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"
#include <iostream>
#include <utility>
#include <cstdlib>

using namespace std;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::school;
using namespace gaia::common;

static const char* g_date_format = "%d %B %Y";

constexpr size_t c_tabsize = 2;

constexpr size_t c_main_menu = 0;
constexpr size_t c_list_menu = 1;
constexpr size_t c_wait_menu = 2;
constexpr size_t c_student_menu = 3;
constexpr size_t c_create_student_menu = 4;
constexpr size_t c_remove_student_menu = 5;
constexpr size_t c_events_menu = 6;
constexpr size_t c_create_event_menu = 7;
constexpr size_t c_edit_event_menu = 8;
constexpr size_t c_registration_menu = 9;
constexpr size_t c_new_registration_menu = 10;
constexpr size_t c_remove_registration_menu = 11;
constexpr size_t c_sim_menu = 12;
constexpr size_t c_char_select_menu = 13;
constexpr size_t c_create_staff_menu = 14;

size_t g_current_menu = c_main_menu;
size_t g_last_menu = c_main_menu;
bool g_continue = true;

void set_curr_menu(size_t menu) {
  g_last_menu = g_current_menu;
  g_current_menu = menu;
}

template <typename T>
bool true_pred (T) {
  return true;
}

void create_world() 
{
  auto_transaction_t txn;

  if (building_t::get_first())
    return;

  int camera_id = 0;
  auto dungeon = building_t::get(building_t::insert_row("Dungeon", camera_id++, true));	

  auto room = room_t::insert_row("B1-01", "Deathday Party Hall", -1, 150);
  dungeon.InBuilding_room_list().insert(room);
  auto party = room_t::get(room);

  room = room_t::insert_row("B1-11", "Potions Classroom", -1, 20);
  dungeon.InBuilding_room_list().insert(room);

  auto potions_class = room_t::get(room);

  room = room_t::insert_row("01-21", "Slytherin Common Room", 1, 50);
  dungeon.InBuilding_room_list().insert(room);

  room = room_t::insert_row("B5-01", "Chamber of Secrets", -5, 12);
  dungeon.InBuilding_room_list().insert(room);

  auto castle = building_t::get(building_t::insert_row("Main Castle", camera_id++, true));
  room = room_t::insert_row("00-00", "Chamber of Reception", 1, 10);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("00-21", "Great Hall", 1, 200);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("00-33", "Transfiguration Classroom", 0, 15);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("01-22", "Muggle Studies Classroom", 1, 20);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("01-42", "History of Magic Classroom", 1, 12);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("03-22", "Database Management Systems Classroom", 1, 5);
  castle.InBuilding_room_list().insert(room);
  auto dbms = room_t::get(room);

  room = room_t::insert_row("04-404", "Study Area", 4, 22);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("05-11", "Art Classroom", 5, 30);
  castle.InBuilding_room_list().insert(room);

  room = room_t::insert_row("05-12", "Music Classroom", 5, 12);
  castle.InBuilding_room_list().insert(room);

  auto library = building_t::get(building_t::insert_row("Library Building", camera_id++, true));
  room = room_t::insert_row("00-00", "Hogwarts Library", 0, 55);
  library.InBuilding_room_list().insert(room);
  auto hlib = room_t::get(room);

  room = room_t::insert_row("01-22", "Disused cupboard", 1, 4);
  library.InBuilding_room_list().insert(room);
  auto cupboard = room_t::get(room);

  auto grounds = building_t::get(building_t::insert_row("The Grounds", camera_id++, true));
  room = room_t::insert_row("00-01", "Quidditch Pitch", 0, 12);
  grounds.InBuilding_room_list().insert(room);
  auto pitch = room_t::get(room);
  
  room = room_t::insert_row("00-22", "Gamekeeper's Hut", 0, 8);
  grounds.InBuilding_room_list().insert(room);
  auto game_hut = room_t::get(room);

  auto tower = building_t::get(building_t::insert_row("Defense Against Dark Arts Tower", camera_id++, true));
  room = room_t::insert_row("B1-22", "Oracle Corporation Licensing Office", -1, 8);
  tower.InBuilding_room_list().insert(room);
  auto oracle = room_t::get(room);

  room = room_t::insert_row("02-12", "Trophy hall", 2, 4);
  tower.InBuilding_room_list().insert(room);

  room = room_t::insert_row("03-11", "Ghoul Studies Classroom", 3, 15);
  tower.InBuilding_room_list().insert(room);

  auto west_tower = building_t::get(building_t::insert_row("West Tower", camera_id++,true));
  room = room_t::insert_row("01-00", "Owlery", 1, 25);
  west_tower.InBuilding_room_list().insert(room);
  auto owl = room_t::get(room);

  room = room_t::insert_row("01-01", "Hazardous Waste Storage", 1, 4);
  west_tower.InBuilding_room_list().insert(room);

  auto clock_tower = building_t::get(building_t::insert_row("Clock Tower", camera_id++, true));
  room = room_t::insert_row("03-00", "Clock Hall", 3, 40);
  clock_tower.InBuilding_room_list().insert(room);

  // create some staff members
  create_staff("Argus", "Filch", -801273600, 631152000);
  create_staff("Albus", "Dumbledore", -2787868800, -441849600);
  create_staff("Filius", "Flitwick", -416793600, 577756800);
  create_staff("Gilderoy", "Lockhart",-187228800, 3110400);
  create_staff("Minerva", "McGonagal", -1490918400, -441849600);
  create_staff("Poppy","Pomfrey", -223257600, 123455);
  create_staff("Quirinus","Quirell", 117849600, 338774400);
  auto t1 = create_staff("Severus","Snape", -314928000, 97372800);
  auto t2 = create_staff("Sybill", "Trelawney",-246672000, 626140800);
  auto t3 = create_staff("Rubeus", "Hagrid", -1296086400, -540864000);
  auto t4 = create_staff("Delores", "Umbridge", -137289600, 694224000);
  
  auto snape = staff_t::get(t1);
  auto sybill = staff_t::get(t2);
  auto hagrid = staff_t::get(t3);
  auto umbridge = staff_t::get(t4);

  // create some events
  // sim start: 775731600
  create_event("Potion Sampling", 775735200, 775738800, potions_class, snape);
  create_event("Prophecies: A Year in Retrospective", 775735200, 775738800, cupboard, sybill);
  create_event("Slytherin Board Game Party", 775735200, 775753200, party, umbridge); 
  create_event("Conflict Resolution the Wizarding Way", 775746000, 775749600, cupboard, sybill);
  create_event("Potion Safety 101", 775746000, 775749600, potions_class, snape);
  create_event("Griffins of the Old World", 775749600, 775756800, game_hut, hagrid);
  create_event("Bird banding Demonstration", 775807200, 775818000, owl, hagrid);
  create_event("CORBA: Let there be ORBs", 775746000, 775756800, hlib, sybill);
  create_event("Super Secret Meeting", 775836000, 775843200, oracle, snape);
  create_event("Triwizard Tournament", 783766800, 783799200, pitch, snape);
  create_event("Expecto Auditus Training", 775818000, 775828800, oracle, umbridge);
  create_event("Career Day", 775789200, 775846800, hlib, umbridge);
  create_event("SQL Workshop", 775818000, 775825200, dbms, sybill);
  
  // create some students and their parents
  create_student("Harry", "Potter", 333849600, "555-5677");

  // Grangers
  auto father = parent_t::get(create_parent("Wendell", "Granger", -586483200, false));
  auto mother = parent_t::get(create_parent("Jean", "Granger", -391651200, true));

  auto s1 = create_student("Hermione", "Granger", 306547200, "555-2359");
  
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Patil twins
  s1 = create_student("Padma", "Patil", 309398400, "555-2223");
  auto s2 = create_student("Parvati", "Patil", 309398400, "555-3333");

  mother = parent_t::get(create_parent("Priyanka", "Patil", -343370643, true));
  father = parent_t::get(create_parent("Aneet", "Patil",  -516516243, false));

  mother.Mother_student_list().insert(s1);
  mother.Mother_student_list().insert(s2);

  father.Father_student_list().insert(s1);
  father.Father_student_list().insert(s2);

  // Weasleys
  father = parent_t::get(create_parent("Arthur", "Weasley", -470361600, false));
  mother = parent_t::get(create_parent("Molly", "Weasley", -636595200, true));

  s1 = create_student("Ron", "Weasley", 320716800, "555-5213");
  s2 = create_student("George", "Weasley", 260236800, "555-2311");
  auto s3 = create_student("William", "Weasley", 260236800, "555-8844");
  auto s4 = create_student("Ginevra", "Weasley", 366336000, "555-1111");

  mother.Mother_student_list().insert(s1);
  mother.Mother_student_list().insert(s2);
  mother.Mother_student_list().insert(s3);
  mother.Mother_student_list().insert(s4);

  father.Father_student_list().insert(s1);
  father.Father_student_list().insert(s2);
  father.Father_student_list().insert(s3);
  father.Father_student_list().insert(s4);

  // Malfoy
  father = parent_t::get(create_parent("Lucius", "Malfoy", -513648000, false));
  mother = parent_t::get(create_parent("Narcissa", "Malfoy", -446947200, true));

  s1 = create_student("Draco", "Malfoy", 329011200, "555-2987");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Diggory
  father = parent_t::get(create_parent("Amos", "Diggory", -632880000, false));
  mother = parent_t::get(create_parent("Helen", "Diggory", -398044800, true));

  s1 = create_student("Cedric", "Diggory", 243216000, "555-5611");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Lovegood
  father = parent_t::get(create_parent("Xenophilius", "Lovegood", -223274643, false));
  mother = parent_t::get(create_parent("Pandora", "Lovegood", -348044800, true));

  s1 = create_student("Luna", "Lovegood", 350939757, "555-2677");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Finnigan
  father = parent_t::get(create_parent("Sean", "Finnigan",  -663741843, false));
  mother = parent_t::get(create_parent("Cara", "Finnigan", -554877843, true));

  s1 = create_student("Seamus", "Finnigan", 350939757, "555-3807");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Chang
  father = parent_t::get(create_parent("Aloysious", "Chang",  -344493843, false));
  mother = parent_t::get(create_parent("Lifang", "Chang", -262413843, true));

  s1 = create_student("Cho", "Chang", 285881486, "555-4921");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Bletchley
  father = parent_t::get(create_parent("Miles", "Bletchley",  -378707314, false));
  mother = parent_t::get(create_parent("Debora", "Bletchley", -508307314, true));

  s1 = create_student("Kevin", "Bletchley", 399670286, "555-2921");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Bell
  father = parent_t::get(create_parent("Roger", "Bell", -633760114, false));
  mother = parent_t::get(create_parent("Penelope", "Bell", -417328114, true));

  s1 = create_student("Katie", "Bell", 313270286, "555-2921");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  // Creevey
  father = parent_t::get(create_parent("Xavier", "Creevey", -412575104, false));
  mother = parent_t::get(create_parent("Elaine", "Creevey", -594187904, true));

  s1 = create_student("Colin", "Creevey", 313270286, "555-2921");
  mother.Mother_student_list().insert(s1);
  father.Father_student_list().insert(s1);

  txn.commit();
}

void reset_world(void) {
  auto_transaction_t txn;

  for (auto e : event_t::list()) {
    auto w = e.writer();
    w.EventActualEnrolled = 0;
    w.Version = 1;
    w.update_row();
  }

  while(auto s = stranger_t::get_first()) {
    if (auto f = s.FirstScanned_FaceScanLog()) {
      f.FirstScanned_stranger_list().erase(s);
    }
    s.delete_row();
  }

  while(auto f = FaceScanLog_t::get_first()) {
    if (auto b = f.Building_building()) {
      b.Building_FaceScanLog_list().erase(f);
    }
    f.delete_row();
  }  

  txn.commit();
}

void init(void) {
  create_world();
  reset_world();
  start_sim();
}

void print_header(const std::string& title) {
  std::cout << "=== " << title << " ===" << std::endl;
}

void print_spacing(size_t spaces)
{
  for (size_t i = 0; i < spaces; ++i) {
    std::cout << "\t";
  }
}

void print_building(building_t& building, size_t spacing = 0)
{
  print_spacing(spacing);
  print_header(building.BuildingName()); 
  std::cout << std::endl;
  print_spacing(spacing);
  std::cout << "BuildingId: " << building.gaia_id() << std::endl;
  print_spacing(spacing);
  std::cout << "CameraId: " << building.CameraId() << std::endl; 
  print_spacing(spacing);
  std::cout << "DoorClosed: " << building.DoorClosed() << std::endl;
}

void print_room (room_t& room, size_t spacing = 0)
{
  print_spacing(spacing);
  std::cout << "RoomId: " << room.gaia_id() << std::endl;
  print_spacing(spacing);
  std::cout << "Room Name: " << room.RoomName() << std::endl; 
  print_spacing(spacing);
  std::cout << "RoomNumber: " << room.RoomNumber() << std::endl;
  print_spacing(spacing);
  std::cout << "Capacity: " << room.Capacity() << std::endl;
  print_spacing(spacing);
  std::cout << "FloorNumber: " << room.FloorNumber() << std::endl;
}

void print_event(event_t& event, size_t spacing = 0)
{
  print_spacing(spacing);
  std::cout << "EventId " << event.gaia_id() << std::endl;
  print_spacing(spacing);
  std::cout << "Event Name: " << event.Name() << std::endl;
  print_spacing(spacing);
  std::cout << "Start Time: ";
  print_time(event.StartTime(), "%Y-%m-%d %I:%M:%S %p");
  std::cout << std::endl;

  print_spacing(spacing);
  std::cout << "End Time: ";
  print_time(event.EndTime(), "%Y-%m-%d %I:%M:%S %p");
  std::cout << std::endl;
 
  print_spacing(spacing);
  auto teacher = event.Teacher_staff();
  auto person = teacher.StaffAsPerson_person();
  std::cout << "Teacher: " << person.FirstName() << " " << person.LastName() << std::endl;

  print_spacing(spacing);
  auto room = event.EventRoom_room();
  auto building = room.InBuilding_building();
  std::cout << "Venue: " << room.RoomName() << ", " << building.BuildingName() << std::endl;  

  print_spacing(spacing);
  std::cout << "Enrollment: " << event.Enrollment() << std::endl;

  print_spacing(spacing);
  std::cout << "Actual Enrollment: " << event.EventActualEnrolled() << std::endl;
}

void print_facescan(FaceScanLog_t f, size_t spacing = 0)
{
  print_spacing(spacing);
  std::cout << "FaceScanLogId: " << f.gaia_id() << std::endl;
  print_spacing(spacing);
  std::cout << "ScanSignature: " << f.ScanSignature() << std::endl;
  print_spacing(spacing);
  std::cout << "ScanTime: ";
  print_time(f.ScanTime(),  "%Y-%m-%d %I:%M:%S %p");
  std::cout << std::endl;

  auto b = f.Building_building();
  print_spacing(spacing);
  std::cout << "Building: " << b.BuildingName();
}

void print_stranger(stranger_t s, size_t spacing = 0)
{
  print_spacing(spacing);
  std::cout << "StrangerId: " << s.gaia_id() << std::endl;
  std::cout << "FaceScanCount: " << s.FaceScanCount() << std::endl;
  std::cout << std::endl;
  print_header("First Scanned");
  print_facescan(s.FirstScanned_FaceScanLog(), spacing + c_tabsize);
}

void list_events(function<bool(event_t&)> f = true_pred<event_t&>)
{
  print_header("Event List");

  auto_transaction_t txn;

  for (auto e: event_t::list()) {
    if (!f)
      continue;
    std::cout << std::endl;
    print_event(e);
    std::cout << std::endl;
  }
}

void list_attendance() 
{
  print_header("Event Underattendance");
  auto_transaction_t txn;

  for (auto e: event_t::list()) {
    if (e.EndTime() > school_now() || e.Enrollment() == e.EventActualEnrolled())
      continue;
    std::cout << std::endl;
    std::cout << "Event Name: " << e.Name() << std::endl;
    std::cout << "Enrollment: " << e.Enrollment() << std::endl;
    std::cout << "Actual Enrollment: " << e.EventActualEnrolled() << std::endl;
    std::cout << std::endl;
  }
}

void list_registrations(function<bool(event_t&)> f = true_pred<event_t&>)
{
  print_header("Event Registration List");

  auto_transaction_t txn;

  for (auto e : event_t::list()) {
    if(!f)
      continue;

    std::cout << std::endl;
    print_header(e.Name());
    std::cout << std::endl << "Registered Students: " << std::endl;

    for (auto r : e.Event_registration_list()) {
      auto s = r.Student_student();
      auto p = s.StudentAsPerson_person();

      print_spacing(c_tabsize);
      std::cout << p.FirstName() << " " << p.LastName() << std::endl;
    }
  }
}

void list_buildings() 
{
  print_header("Building List");
  
  auto_transaction_t txn;

  for (auto b: building_t::list()) {
    std::cout << std::endl;
    print_building(b);
    std::cout << std::endl;
    print_spacing(c_tabsize);
    print_header("Rooms");

    for (auto r: b.InBuilding_room_list()) {
      std::cout << std::endl;
      print_room(r, c_tabsize);
      std::cout << std::endl;
    }
  }
}

void list_facescans()
{
  print_header("FaceScan List");
  auto_transaction_t txn;

  for (auto f: FaceScanLog_t::list()) {
    std::cout << std::endl;
    print_facescan(f);
    std::cout << std::endl;
  }
}

void list_strangers()
{
  print_header("Stranger List");
  auto_transaction_t txn;

  for (auto s:stranger_t::list()) {
    std::cout << std::endl;
    print_stranger(s);
    std::cout << std::endl;
  }
}

void print_person(person_t& person, size_t spacing = 0) {
  print_spacing(spacing);
  std::cout << "FirstName: " << person.FirstName() << std::endl;
  print_spacing(spacing);
  std::cout << "LastName: " << person.LastName() << std::endl;
  print_spacing(spacing);
  std::cout << "Birthday: ";
  print_time(person.BirthDate(), g_date_format);
  std::cout << std::endl;
}

void print_staff(staff_t& staff, size_t spacing = 0) {

  auto p = staff.StaffAsPerson_person();
  
  print_spacing(spacing);
  std::cout << "StaffId: " << staff.gaia_id() << std::endl;
  print_person(p, spacing);
  print_spacing(spacing);
  std::cout << "Hired date: ";
  print_time(staff.HiredDate(), "%Y-%B-%d");
  std::cout << std::endl;
}

void print_student(student_t& student, size_t spacing = 0) {
  auto p = student.StudentAsPerson_person();
  print_spacing(spacing);
  std::cout << "StudentId: " << student.gaia_id() << std::endl;
  print_person(p, spacing);
  print_spacing(spacing);
  std::cout << "Number: " << student.Number() << std::endl;
  print_spacing(spacing);
}

void list_staff() {
  auto_transaction_t txn;
  size_t counter = 0;
  print_header("Staff List");
  for (auto s: staff_t::list()) {
    std::cout << std::endl;
    print_staff(s);
    std::cout << std::endl;

    counter++;
  }

  std::cout << counter << " staff" << std::endl;
};

void list_students() {
  auto_transaction_t txn;
  size_t counter = 0;

  print_header("Student List");

  for (auto s: student_t::list()) {
    std::cout << std::endl;
    print_student(s);
    std::cout << std::endl;

    print_spacing(c_tabsize);
    print_header("Mother");
    std::cout << std::endl;
    if (auto m = s.Mother_parent()) {
       auto p = m.ParentAsPerson_person();
       print_person(p, c_tabsize);
    } else {
       print_spacing(c_tabsize);
       std::cout << "None" << std::endl;
       std::cout << std::endl;
    }
    std::cout << std::endl;
    print_spacing(c_tabsize);
    print_header("Father");
    std::cout << std::endl;
    if (auto f = s.Father_parent()) {
       auto p = f.ParentAsPerson_person();
       print_person(p, c_tabsize);
    } else {
       print_spacing(c_tabsize);
       std::cout << "None" << std::endl;
       std::cout << std::endl;
    }
    counter++;
  }

  std::cout << counter << " students" << std::endl;
}

void list_parents() {
  auto_transaction_t txn;
  size_t counter = 0;

  print_header("Parent List");

  for (auto p: parent_t::list()) {
    std::cout << "ParentId: " <<p.gaia_id() << std::endl;
    auto person = p.ParentAsPerson_person();
    print_person(person);

    // print children
    std::cout << std::endl;
    print_spacing(c_tabsize);
    print_header("Children");
    std::cout << std::endl;
    for (auto s: p.Mother_student_list()) {
      print_student(s, c_tabsize);
      std::cout << std::endl;
    }

    for (auto s: p.Father_student_list()) {
      print_student(s, c_tabsize);
      std::cout << std::endl;
    }

    counter++;
  }

  std::cout << counter << " parents" << std::endl;
}

void print_now() {
  auto_transaction_t txn;

  std::cout << "The time is now: ";
  print_time(school_now(), "%Y-%m-%d %I:%M:%S %p");
  std::cout << std::endl;
}

void print_whoami() {
  auto_transaction_t txn;
  std::cout << school_whoami();
}

template <typename T>
T get_value(const std::string& prompt, std::function<bool(T)> validation = [](T) {return true;}) {
  T input;

  std::cout << prompt;

  std::cin >> input;

  while(std::cin.fail() || !validation(input)) {
     std::cout << "Invalid Entry" << std::endl;
     std::cin.clear();
     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
     
     std::cout << prompt;
     std::cin >> input;
  }

  return input;
}

std::time_t get_date(void) {
  time_t raw_time;
  std::time(&raw_time);
  struct tm* timeinfo = std::gmtime(&raw_time);

  timeinfo->tm_year = get_value<int>("Year: ")  - 1900;
  timeinfo->tm_mon = get_value<int>("Month: ", [](int m) {return m > 0 && m <= 12; }) - 1;
  timeinfo->tm_mday = get_value<int>("Day of Month: ", [](int dd) {return dd > 0 && dd <= 31; });

  timeinfo->tm_hour = 0;
  timeinfo->tm_min = 0; 
  timeinfo->tm_sec = 0;

  return ::timegm(timeinfo);
}

std::time_t get_date_and_time(void) {
  time_t raw_time;
  std::time(&raw_time);
  struct tm* timeinfo = std::gmtime(&raw_time);

  timeinfo->tm_year = get_value<int>("Year: ")  - 1900;
  timeinfo->tm_mon = get_value<int>("Month: ", [](int m) {return m > 0 && m <= 12; }) - 1;
  timeinfo->tm_mday = get_value<int>("Day of Month: ", [](int dd) {return dd > 0 && dd <= 31; });

  timeinfo->tm_hour = get_value<int>("Hours since midnight: ", [](int hh) {return hh >= 0 && hh < 23;});
  timeinfo->tm_min = 0; 
  timeinfo->tm_sec = 0;

  return ::timegm(timeinfo);
}

std::time_t get_time(time_t date, std::function<bool(time_t)> validate = true_pred<time_t>) {
  struct tm* timeinfo = std::gmtime(&date);
  time_t retval;

  timeinfo->tm_hour = get_value<int>("Hours: ", [](int hh) {return hh >= 0 && hh < 23;});
  timeinfo->tm_min = 0;
  timeinfo->tm_sec = 0;

  retval = ::timegm(timeinfo); 

  while(!validate(retval)) {
    std::cout << "Invalid Time" << std::endl;

    timeinfo->tm_hour = get_value<int>("Hours: ", [](int hh) {return hh >= 0 && hh < 23;});
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;

    retval = ::timegm(timeinfo);
  }

  return retval;
}

std::string get_line(const std::string& prompt) {
  std::string line;
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::cout << prompt;
  std::getline (std::cin, line);

  while(std::cin.fail()) {
    std::cout << "Invalid Entry\n" << std::endl;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << prompt;
    std::getline (std::cin, line);
  }

  return line;
}

void key_continue() {
  get_line("Press return to continue... ");
}

gaia_id_t get_student() {
  size_t counter = 0;

  std::map<size_t, gaia_id_t> choice_id_map;

  for (auto s: student_t::list()) {
    ++counter;
    std::cout << counter << ") ";
    auto p = s.StudentAsPerson_person();
    std::cout << p.FirstName() << " " << p.LastName() << std::endl;

    choice_id_map[counter] = s.gaia_id();
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= counter;});
  
  return choice_id_map[choice];
};

gaia_id_t get_mother() {
  size_t counter = 0;

  std::map<size_t, gaia_id_t> choice_id_map;

  for (auto s: parent_t::list()) {
    if (!s.MotherFather())
      continue;
    ++counter;
    std::cout << counter << ") ";
    auto p = s.ParentAsPerson_person();
    std::cout << p.FirstName() << " " << p.LastName() << std::endl;

    choice_id_map[counter] = s.gaia_id();
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= counter;});
  
  return choice_id_map[choice];
};

gaia_id_t get_father() {
  size_t counter = 0;

  std::map<size_t, gaia_id_t> choice_id_map;

  for (auto s: parent_t::list()) {
    if (s.MotherFather())
      continue;
    auto p = s.ParentAsPerson_person();
    ++counter;
    std::cout << counter << ") ";
    std::cout << p.FirstName() << " " << p.LastName() << std::endl;

    choice_id_map[counter] = s.gaia_id();
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= counter;});
  
  return choice_id_map[choice];
};

gaia_id_t get_staff() {
  size_t counter = 0;

  std::map<size_t, gaia_id_t> choice_id_map;

  for (auto s: staff_t::list()) {
    ++counter;
    std::cout << counter << ") ";
    auto p = s.StaffAsPerson_person();
    std::cout << p.FirstName() << " " << p.LastName() << std::endl;

    choice_id_map[counter] = s.gaia_id();
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= counter;});
  
  return choice_id_map[choice];
};

gaia_id_t get_building() {
  size_t counter = 0;

  std::map<size_t, gaia_id_t> choice_id_map;

  for (auto b: building_t::list()) {
    ++counter;
    std::cout << counter << ") ";
    std::cout << b.BuildingName() << std::endl;

    choice_id_map[counter] = b.gaia_id();
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= counter;});
  
  return choice_id_map[choice];
};

gaia_id_t get_room() {
  size_t counter = 0;

  std::map<size_t, gaia_id_t> choice_id_map;

  for (auto r: room_t::list()) {
    ++counter;
    std::cout << counter << ") ";
    std::cout << r.RoomName() << std::endl;

    choice_id_map[counter] = r.gaia_id();
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= counter;});
  
  return choice_id_map[choice];
};

void wait_menu(void) {
  std::system("clear");  
  print_header("The Waiting Game");
  std::cout << "1) Wait one hour" << std::endl;
  std::cout << "2) Wait to next day (9 am)" << std::endl;
  std::cout << "3) Use Time Turner (time travel)" << std::endl;
  std::cout << "0) Back" << std::endl;
  std::cout << std::endl;
  print_now();
  std::cout << std::endl;

  std::cout << "You stand around. Do you have somewhere to be?" << std::endl;

  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c <= 3;});

  switch(choice) {
    case 1:
    advance_hour();
    break;
    case 2:
    advance_day();
    break;
    case 3:
    {
      std::cout << std::endl;
      std::cout << "Select Date and Time" << std::endl;
      time_t t = get_date_and_time();
      std::cout << "The time turner turns. It is now ";
      print_time(t, "%Y-%m-%d %I:%M:%S %p");
      std::cout << std::endl << std::endl;
      key_continue();
      time_travel(static_cast<int64_t>(t));
    }
    break;
    case 0:
    break;
  }
  set_curr_menu(c_sim_menu);
}

void list_menu(void) {
  std::system("clear");

  print_header("List Objects");
  std::cout << "1) List Buildings" << std::endl;
  std::cout << "2) List Staff" << std::endl;
  std::cout << "3) List Students" << std::endl;
  std::cout << "4) List Parents" << std::endl;
  std::cout << "5) List Events" << std::endl;
  std::cout << "6) List Registrations" << std::endl;
  std::cout << "7) List Face Scans" << std::endl;
  std::cout << "8) List Strangers" << std::endl;
  std::cout << "0) Back" << std::endl;

  std::cout << std::endl;
  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c <= 8;});

  switch(choice) {
    case 1:
    list_buildings();
    std::cout << std::endl;
    key_continue();
    break;
    case 2:
    list_staff();
    std::cout << std::endl;
    key_continue();
    break;
    case 3:
    list_students();
    std::cout << std::endl;
    key_continue();
    break;
    case 4:
    list_parents();
    std::cout << std::endl;
    key_continue();
    break;
    case 5:
    list_events();
    std::cout << std::endl;
    key_continue();
    break;
    case 6:
    list_registrations(); 
    std::cout << std::endl;
    key_continue();
    break;
    case 7:
    list_facescans();
    std::cout << std::endl;
    key_continue();
    break;
    case 8:
    list_strangers();
    std::cout << std::endl;
    key_continue();
    break;
    case 0:
    set_curr_menu(g_last_menu);
    break;
  }
}

void create_staff_menu(void) {
  print_header("Create Staff");
  std::string firstname = get_line("First Name:");
  std::string lastname = get_line("Last Name:");
  std::cout << "Input Birthdate" << std::endl;
  time_t dob = get_date();

  auto_transaction_t txn;
  create_staff(firstname, lastname, dob, school_now());
  txn.commit();

  set_curr_menu(c_main_menu);
}

void create_student_menu(void) {
  print_header("Create Student");
  std::string firstname = get_line("First Name:");
  std::string lastname = get_line("Last Name:");
  std::cout << "Input Birthdate" << std::endl;
  time_t dob = get_date();
  std::string number = get_line("Number: ");

  auto_transaction_t txn;
  auto s = create_student(firstname, lastname, dob, number);

  std::cout << "Mother:" << std::endl;
  std::cout << "1) Create new" << std::endl;
  std::cout << "2) Select existing" << std::endl;
  std::cout << "3) None" << std::endl;

  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c){ return c > 0 && c <= 3; });

  switch(choice) {
    case 1:
    {
    firstname = get_line("First Name: ");
    std::cout << "input Birthdate" << std::endl;
    dob = get_date();
    auto p = parent_t::get(create_parent(firstname, lastname, dob, true));
    p.Mother_student_list().insert(s);
    }
    break;
    case 2:
    {
      std::cout << "Select: " << std::endl;
      auto mother = get_mother();
      auto p = parent_t::get(mother);
      p.Mother_student_list().insert(s);
    }
    break;
    case 3:
    break;
  }

  std::cout << std::endl;
  std::cout << "Father:" << std::endl;
  std::cout << "1) Create new" << std::endl;
  std::cout << "2) Select existing" << std::endl;
  std::cout << "3) None" << std::endl;

  choice = get_value<size_t>("\nChoice: ", [](size_t c){ return c > 0 && c <= 3; });
  
  switch(choice) {
    case 1:
    {
    firstname = get_line("First Name: ");
    std::cout << "input Birthdate" << std::endl;
    dob = get_date();
    auto p = parent_t::get(create_parent(firstname, lastname, dob, false));
    p.Father_student_list().insert(s);
    }
    break;
    case 2:
    {
      std::cout << "Select: " << std::endl;
      auto father = get_father();
      auto p = parent_t::get(father);
      p.Father_student_list().insert(s);
    }
    break;
    case 3:
    break;
  }
  
  txn.commit();
  set_curr_menu(c_student_menu);
}

void deregister_all_events(student_t& s) {
  size_t count_reg;
  do {
    registration_t reg_to_delete;
    count_reg = 0;
    for (auto r : s.Student_registration_list()) {
      ++count_reg;
      reg_to_delete = r;
    }
    if (count_reg) {
      auto e = reg_to_delete.Event_event();
      e.Event_registration_list().erase(reg_to_delete);
      s.Student_registration_list().erase(reg_to_delete);
      reg_to_delete.delete_row();
    }
  } while(count_reg > 0);
}

void remove_student_menu(void) {
  print_header("Remove Student");
  std::cout << "Select student for removal: " << std::endl;

  auto_transaction_t txn;
  
  gaia_id_t id = get_student();
  auto child = gaia::school::student_t::get(id);

  deregister_all_events(child);

  auto person = child.StudentAsPerson_person();
  person.StudentAsPerson_student_list().erase(child);
  person.delete_row();

  auto f = child.Father_parent();
  bool father_has_no_child = true;
  
  for (auto s : f.Father_student_list()) {
    if (s.gaia_id() != id) {
      father_has_no_child = false;
      break;
    }
  }
  if (f) {
    f.Father_student_list().erase(child);
  }
  if (f && father_has_no_child) {
    std::cout << "Deleting father... "<< std::endl;
    person = f.ParentAsPerson_person();
    person.ParentAsPerson_parent_list().erase(f);
    person.delete_row();
    f.delete_row();
  }
  auto m = child.Mother_parent();
  bool mother_has_no_child = true;
  for (auto s : m.Mother_student_list()) {
    if (s.gaia_id() != id) {
      mother_has_no_child = false;
      break;
    }
  }
  if (m)
     m.Mother_student_list().erase(child);
  if (m && mother_has_no_child) {
     std::cout << "Deleting mother... "<< std::endl;
     person = m.ParentAsPerson_person();
     person.ParentAsPerson_parent_list().erase(m);
     person.delete_row();
     m.delete_row();
  }
  child.delete_row(id);

  txn.commit();

  set_curr_menu(c_student_menu);
}

void student_menu(void) {
  std::system("clear");
  print_header("Student Registration");
  std::cout << std::endl;
  std::cout << "1) Create Student" << std::endl;
  std::cout << "2) Remove Student" << std::endl;
  std::cout << "3) List Students" << std::endl;
  std::cout << "4) List Parents" << std::endl;
  std::cout << "0) Return to main menu" << std::endl;
  std::cout << std::endl;

  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c >=0 && c <= 4;});

  switch(choice) {
    case 1:
    set_curr_menu(c_create_student_menu);
    break;
    case 2:
    set_curr_menu(c_remove_student_menu);
    break;
    case 3:
    list_students();
    key_continue();
    break;
    case 4:
    list_parents();
    key_continue();
    break;
    case 0:
    set_curr_menu(c_main_menu);
    break;
  }
}

bool evt_overlap(event_t& e1, event_t& e2) {
  return e1.EndTime() >= e2.StartTime() && e2.EndTime() >= e1.StartTime();
}

bool staff_avail(staff_t& staff, event_t& evt) {
  bool free = true;

  for (auto e: staff.Teacher_event_list()) {
    if (evt_overlap(e, evt)) {
      free = false;
      break;
    }
  }

  return free;
}

bool room_avail(room_t& room, event_t& evt) {
  if (room.Capacity() < evt.Enrollment())
    return false;

  bool free = true;

  for (auto e: room.EventRoom_event_list()) {
    if (evt_overlap(e, evt)) {
      free = false;
      break;
    }
  }

  return free;
}

void create_event_menu (void) {
  print_header("Create Event");
  std::cout << std::endl;
  string evt_name = get_line("Event Name: ");
  std::cout << std::endl;

  std::cout << "Event Date: " << std::endl;
  time_t date = get_date();

  std::cout << "Start Time: " << std::endl;
  time_t start = get_time(date);

  std::cout << "End Time: " << std::endl;
  time_t end = get_time(date, [=](time_t t) { return t > start;});

  {
  auto_transaction_t txn;

  auto id = event_t::insert_row(evt_name.c_str(), 1, start, end, 0, 0);
  auto e = event_t::get(id);
  
  std::cout << std::endl;
  std::cout << "Select Teacher " << std::endl;
  std::cout << std::endl;

  std::map<size_t, gaia_id_t> choice_staff;
  size_t ctr = 0;
  for (auto t: staff_t::list()) {
    if (!staff_avail(t, e))
      continue;
    
    ++ctr;
    auto p = t.StaffAsPerson_person();
    std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;

    choice_staff[ctr] = t.gaia_id();
  }

  if (ctr == 0) {
    std::cout << "No staff available for event" << std::endl;
    set_curr_menu(c_events_menu);
    key_continue();
    return;
  }

  size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <=ctr;});
  auto t = staff_t::get(choice_staff[choice]);
  t.Teacher_event_list().insert(e);

  std::cout << std::endl;
  std::cout << "Select Room " << std::endl;
  std::cout << std::endl;

  ctr = 0;
  std::map<size_t, gaia_id_t> choice_room;
  for (auto r: room_t::list()) {
    if (!room_avail(r, e)) {
      continue;
    }

    ++ctr;
    std::cout << ctr << ") " << r.RoomName() << std::endl;
    choice_room[ctr] = r.gaia_id();
  }

  if (ctr == 0) {
    std::cout << "No room available for event" << std::endl;
    set_curr_menu(c_events_menu);
    key_continue();
    return;
  }

  choice = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <=ctr;});
  auto r = room_t::get(choice_room[choice]);
  r.EventRoom_event_list().insert(e);
  
  txn.commit();
  
  }

  set_curr_menu(c_events_menu);
}

void edit_event_menu(void) {
  print_header("Edit Event");
  std::cout << std::endl;

  std::cout << "Select Event to Modify" << std::endl;
  {
    auto_transaction_t txn;
    time_t now = school_now();

    map <size_t, gaia_id_t> choice_event;
    size_t ctr = 0;
    for (auto e: event_t::list()) {
      if (e.StartTime() <= now)
        continue;

      ctr++;
      std::cout << ctr << ") " << e.Name() << std::endl;
      choice_event[ctr] = e.gaia_id();
    }

    if (ctr == 0) {
      std::cout << "No events available to modify" << std::endl;
      set_curr_menu(c_events_menu);
      key_continue();
      return;
    }

    size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= ctr;});
    auto id = choice_event[choice];
    auto e = event_t::get(id);

    std::cout << std::endl;
    std::cout << "Event selected: " << std::endl;
    std::cout << std::endl;
    print_event(e);
    std::cout << std::endl;
    std::cout << "1) Reschedule" << std::endl;
    std::cout << "2) Change Venue" << std::endl;
    std::cout << "3) Change Staff" << std::endl;
    std::cout << "4) Cancel Event" << std::endl;
    std::cout << "0) Back" << std::endl;

    choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c >= 0 && c <= 4;});
   
    switch(choice) {
      case 1:
      {	
	int64_t diff = e.EndTime() - e.StartTime(); 
	std::cout << std::endl; 
        std::cout << "Event Date: " << std::endl;
        time_t date = get_date();

        std::cout << "Start Time: " << std::endl;
        time_t start = get_time(date);


        time_t end = start + diff;
        
	auto w = e.writer();
	w.StartTime = start;
	w.EndTime = end;
	w.Version = e.Version() + 1; // bump the version to trigger update
	w.update_row();
      }
      break;
      case 2:
      {
        ctr = 0;
        std::map<size_t, gaia_id_t> choice_room;
	std::cout << "Select new room" << std::endl;
        for (auto r: room_t::list()) {
          if (!room_avail(r, e)) {
            continue;
          }

        ++ctr;
        std::cout << ctr << ") " << r.RoomName() << std::endl;
        choice_room[ctr] = r.gaia_id();
        }

        if (ctr == 0) {
          std::cout << "No room available for event" << std::endl;
          set_curr_menu(c_events_menu);
          return;
        }


        choice = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <=ctr;});
	auto old_room = e.EventRoom_room();
	old_room.EventRoom_event_list().erase(e);
        auto r = room_t::get(choice_room[choice]);
        r.EventRoom_event_list().insert(e);
	auto w = e.writer();
	w.Version = e.Version() + 1;
	w.update_row();
      }
      break;
      case 3:
      {
        ctr = 0;
        std::map<size_t, gaia_id_t> choice_staff;
	std::cout << "Select new staff" << std::endl;
        for (auto s: staff_t::list()) {
          if (!staff_avail(s, e)) {
            continue;
          }

        ++ctr;
	auto p = s.StaffAsPerson_person();
        std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;
        choice_staff[ctr] = s.gaia_id();
        }

        if (ctr == 0) {
          std::cout << "No staff available for event" << std::endl;
          set_curr_menu(c_events_menu);
          return;
        }


        choice = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <=ctr;});
	auto old_staff = e.Teacher_staff();
	old_staff.Teacher_event_list().erase(e);
        auto s = staff_t::get(choice_staff[choice]);
        s.Teacher_event_list().insert(e);
	auto w = e.writer();
	w.update_row();
      }
      break;
      case 4:
      {
        auto w = e.writer();
	w.Version = 0;
	w.update_row();
      }
      break;
      case 0:
      {
	set_curr_menu(c_events_menu);
	return;
      }
      break;
    }

    txn.commit();
  }
  key_continue();
  set_curr_menu(c_events_menu);
}

void events_menu(void) {
  std::system("clear");
  print_header("Event Management");
  std::cout << std::endl;
  std::cout << "1) Create Event" << std::endl;
  std::cout << "2) Modify Event" << std::endl;
  std::cout << "3) List Events" << std::endl;
  std::cout << "4) List Registrations" << std::endl;
  std::cout << "5) Event Registration" << std::endl;
  std::cout << "6) Check Event Attendance" << std::endl;
  std::cout << "0) Return to main menu" << std::endl;

  std::cout << std::endl;
  print_now();
  std::cout << std::endl;

  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c >= 0 && c <= 6;});

  switch(choice) {
    case 1:
    set_curr_menu(c_create_event_menu);
    break;
    case 2:
    set_curr_menu(c_edit_event_menu);
    break;
    case 3:
    list_events();
    key_continue();
    break;
    case 4:
    list_registrations();
    key_continue();
    case 5:
    set_curr_menu(c_registration_menu);
    break;
    case 6:
    list_attendance();
    key_continue();
    break;
    case 0:
    set_curr_menu(c_main_menu);
    break;
  }
}

void new_registration_menu(void) {
  std::system("clear");
  print_header("Register for Events"); 
  std::cout << std::endl;
  print_now();
  std::cout << std::endl;

  {
    auto_transaction_t txn;
    std::cout << "Select Event " << std::endl;
    time_t now = school_now();
    size_t ctr = 0;
    std::map<size_t, gaia_id_t> choice_event;
    for (auto evt: event_t::list()) {
      if(evt.StartTime() <= now)
        continue;
      ++ctr;

      std::cout << ctr << ") " << evt.Name() << std::endl;
      choice_event[ctr] = evt.gaia_id();
    }

    if (ctr == 0) {
      std::cout << "No events available" << std::endl;
      set_curr_menu(c_events_menu);
      key_continue();
      return;
    }
    size_t choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= ctr;});
    auto id = choice_event[choice];
    auto e = event_t::get(id);
   
    std::cout << std::endl; 
    std::cout << "Select Student" << std::endl;

    ctr = 0;
    std::map<size_t, gaia_id_t> choice_student;
    for (auto s : student_t::list())
    {
      bool already_registered = false;
      for (auto r: s.Student_registration_list()) {
        if(r.Event_event() == e) {
          already_registered = true;
	  break;
	}
      }
      if (already_registered)
        continue;

      ++ctr;
      auto p = s.StudentAsPerson_person();
      std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;
      choice_student[ctr] = s.gaia_id();
    }
 
    if (ctr == 0) {
      std::cout << "No students available" << std::endl;
      set_curr_menu(c_events_menu);
      key_continue();
      return;
    }

    choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= ctr;});
    id = choice_student[choice];
    auto s = student_t::get(id);

    register_student(s, e);

    txn.commit();
  }

  key_continue();
  set_curr_menu(c_registration_menu);
}

void remove_registration_menu(void) {
  std::system("clear");
  std::cout << "Select Student" <<std::endl;

  auto_transaction_t txn;

  size_t ctr = 0;
  std::map<size_t, gaia_id_t> choice_student;
  for (auto s : student_t::list())
  {
    if (s.Student_registration_list().begin() == s.Student_registration_list().end())
      continue;

    ++ctr;
    auto p = s.StudentAsPerson_person();
    std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;
    choice_student[ctr] = s.gaia_id();
  }
 
  if (ctr == 0) {
    std::cout << "No students available" << std::endl;
    set_curr_menu(c_events_menu);
    key_continue();
    return;
  }

  auto choice = get_value<size_t>("\nChoice: ", [=](size_t c) {return c > 0 && c <= ctr;});
  auto id = choice_student[choice];
  auto s = student_t::get(id);
  
  ctr = 0;
  std::map<size_t, gaia_id_t> choice_registration;
  
  for (auto r : s.Student_registration_list()) {
    ++ctr;
    auto evt = r.Event_event();
    std::cout << ctr << ") " << evt.Name() << std::endl; 
    choice_registration[ctr] = r.gaia_id();
  }

  choice = get_value<size_t>("\nChoice: ", [=](size_t c ) {return c > 0 && c <= ctr;});
  id = choice_registration[choice];
  auto reg = registration_t::get(id);

  auto evt = reg.Event_event();
  evt.Event_registration_list().erase(reg);
  s.Student_registration_list().erase(reg);

  reg.delete_row();
  
  txn.commit();

  key_continue();
  set_curr_menu(c_registration_menu);
}

void registration_menu(void) {
  std::system("clear");
  print_header("Register for Events");
  std::cout << std::endl;
  std::cout << "1) New Registration" << std::endl;
  std::cout << "2) Remove Registration" << std::endl;
  std::cout << "3) List Registrations" << std::endl;
  std::cout << "4) List Events" << std::endl;
  std::cout << "5) Event Management" << std::endl;
  std::cout << "0) Return to Main Menu" << std::endl;
  std::cout << std::endl;
  print_now();
  std::cout << std::endl;

  size_t choice = get_value<size_t>("\nChoice: " ,[](size_t c) {return c >= 0 && c <= 5;});

  switch(choice) {
    case 1:
    set_curr_menu(c_new_registration_menu);
    break;
    case 2:
    set_curr_menu(c_remove_registration_menu);
    break;
    case 3:
    list_registrations();
    key_continue();
    break;
    case 4:
    list_events();
    key_continue();
    break;
    case 5:
    set_curr_menu(c_events_menu);
    break;
    case 0:
    set_curr_menu(c_main_menu);
    break;
  }
}

void sim_menu (void) {
  std::system("clear");
  cout << std::endl;
  print_whoami();
  cout << std::endl << std::endl;
  std::cout << "1) Go to building" << std::endl;
  std::cout << "2) Wait" << std::endl;
  std::cout << "3) Look at Mirror of FaceSignaturus" << std::endl;
  std::cout << "4) Select Another Character" << std::endl;
  std::cout << "5) List Objects" << std::endl;
  std::cout << "6) Reset World" << std::endl;
  std::cout << "0) Back to Main Menu" << std::endl;
  std::cout << std::endl;
  print_now();
  std::cout << std::endl;

  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c >= 0 && c <= 6;});
  switch(choice) {
    case 1:
      {
      auto_transaction_t txn;

      size_t ctr = 0;
      std::map<size_t, gaia_id_t> choice_building;
      for (auto b: building_t::list()) {
        ++ctr;
	std::cout << ctr << ") " << b.BuildingName() << std::endl;
	choice_building[ctr] = b.gaia_id();
      }

      size_t cc = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <= ctr;});
      auto build = building_t::get(choice_building[cc]);

      create_facescan(build);
      txn.commit();
      key_continue();
      }
      break;
    case 2:
      set_curr_menu(c_wait_menu);
      break;
    case 3:
      {
      auto_transaction_t txn;
      std::cout << "You look in the mirror and see " << school_myface();
      std::cout << "." << std::endl << "It's your face signature." << std::endl;
      key_continue();
      }
      break;
    case 4:
      set_curr_menu(c_char_select_menu);
      break;
    case 5:
      set_curr_menu(c_list_menu);
      break;
    case 6:
      reset_world();
      std::cout << "World reset..." << std::endl;
      key_continue();
      break;
    case 0:
      set_curr_menu(c_main_menu);
      break;
  }
}

void char_select_menu(void) {
  std::system("clear");
  print_header("Character select");
  std::cout << "1) Select Student" << std::endl;
  std::cout << "2) Select Staff" << std::endl;
  std::cout << "3) Select Parent" << std::endl;
  std::cout << "4) Select Voldemort (Stranger)" << std::endl;
  std::cout << "5) Select Bellatrix (Stranger)" << std::endl;
  std::cout << "0) Back" << std::endl;

  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c >= 0 && c <= 5;});
  switch(choice) {
    case 1:
      {
      auto_transaction_t txn;
      std::cout << std::endl << "Who would you want to be? " << std::endl;
      size_t ctr = 0;
      std::map<size_t, gaia_id_t> choice_person;
      for (auto s : student_t::list()) {
        ++ctr;
	auto p = s.StudentAsPerson_person();
	std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;
        choice_person[ctr] = p.gaia_id();	
      }

      if (ctr == 0) {
        std::cout << "No students available" << std::endl;
	return;
      }

      size_t cc = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <= ctr;});
      auto p = person_t::get(choice_person[cc]);
      play_as_person(p);
      txn.commit();
      }
      break;
    case 2:
      {
      auto_transaction_t txn;
      std::cout <<  std::endl << "Who would you want to be? " << std::endl;
      size_t ctr = 0;
      std::map<size_t, gaia_id_t> choice_person;
      for (auto s : staff_t::list()) {
        ++ctr;
	auto p = s.StaffAsPerson_person();
	std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;
        choice_person[ctr] = p.gaia_id();	
      }

      if (ctr == 0) {
        std::cout << "No staff available" << std::endl;
	return;
      }

      size_t cc = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <= ctr;});
      auto p = person_t::get(choice_person[cc]);
      play_as_person(p);
      txn.commit();
      }
      break;
    case 3: 
      {
      auto_transaction_t txn;
      std::cout <<  std::endl << "Who would you want to be? " << std::endl;
      size_t ctr = 0;
      std::map<size_t, gaia_id_t> choice_person;
      for (auto s : parent_t::list()) {
        ++ctr;
	auto p = s.ParentAsPerson_person();
	std::cout << ctr << ") " << p.FirstName() << " " << p.LastName() << std::endl;
        choice_person[ctr] = p.gaia_id();	
      }

      if (ctr == 0) {
        std::cout << "No parent available" << std::endl;
	return;
      }

      size_t cc = get_value<size_t>("\nChoice: ", [=](size_t c){return c > 0 && c <= ctr;});
      auto p = person_t::get(choice_person[cc]);
      play_as_person(p);
      txn.commit();
      }
      break;
    case 4:
      {
	auto_transaction_t txn;
        play_as_stranger();
	txn.commit();
      }
      break;
    case 5:
      {
        auto_transaction_t txn;
	play_as_stranger(2, "Bellatrix Lestrange");
	txn.commit();
      }
      break;
    case 0:
      break;
  }
  set_curr_menu(c_sim_menu);
}

void main_menu(void) {
  std::system("clear");  
  print_header("Welcome to HOGWARTS");
  std::cout << std::endl;
  std::cout << "1) List Objects" << std::endl;
  std::cout << "2) Student Management" << std::endl;
  std::cout << "3) Events Management" << std::endl;
  std::cout << "4) Event Registration" << std::endl;
  std::cout << "5) Staff Registration" << std::endl;
  std::cout << "6) Simulate Character" << std::endl;
  std::cout << "0) Quit" << std::endl;
  std::cout << std::endl;
  print_now();
  std::cout << std::endl;
  size_t choice = get_value<size_t>("\nChoice: ", [](size_t c) {return c >= 0 && c <= 6;});

  switch(choice) {
    case 1:
      set_curr_menu(c_list_menu);
      break;
    case 2:
      set_curr_menu(c_student_menu);
      break;
    case 3:
      set_curr_menu(c_events_menu);
      break;
    case 4:
      set_curr_menu(c_registration_menu);
      break;
    case 5:
      set_curr_menu(c_create_staff_menu);
      break;
    case 6:
      set_curr_menu(c_sim_menu);
      break;
    case 0:
      g_continue = false;
  }
}

int main (void) {
  gaia::system::initialize();

  init();

  while (g_continue) {
    switch(g_current_menu) {
      case c_main_menu:
      main_menu();
      break;
      case c_list_menu:
      list_menu();
      break;
      case c_wait_menu:
      wait_menu();
      break;
      case c_student_menu:
      student_menu();
      break;
      case c_create_student_menu:
      create_student_menu();
      break;
      case c_remove_student_menu:
      remove_student_menu();
      break;
      case c_events_menu:
      events_menu();
      break;
      case c_create_event_menu:
      create_event_menu();
      break;
      case c_edit_event_menu:
      edit_event_menu();
      break;
      case c_registration_menu:
      registration_menu();
      break;
      case c_new_registration_menu:
      new_registration_menu();
      break;
      case c_remove_registration_menu:
      remove_registration_menu();
      break;
      case c_sim_menu:
      sim_menu();
      break;
      case c_char_select_menu:
      char_select_menu();
      break;
      case c_create_staff_menu:
      create_staff_menu();
      break;
    }
  }

  gaia::system::shutdown();
  return 0;
}
