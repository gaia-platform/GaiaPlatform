#include "gaia_school.h"
#include "phone.h"

#include "gaia/system.hpp"

#include <iostream>
#include <unistd.h>

using namespace gaia::db;
using namespace gaia::school;
using namespace gaia::common;
using namespace gaia::direct_access;

constexpr size_t MAX_STUDENTS = 16;

void msg_number(const std::string& number, const std::string& msg) {
  gaia::school::msg_t::insert_row(number.c_str(), msg.c_str());
}

/*
extern "C" {
  void initialize_rules() {
  }
}*/

int main() {
  gaia::system::initialize();
  std::cout << "YO for wizards" << std::endl;
  std::cout << "Send TO? " << std::endl;

  gaia_id_t choice_to_id[MAX_STUDENTS];

  size_t choice = 1;
  size_t my_choice;
  {
    auto_transaction_t txn;
  for (auto s: student_t::list()) {
    if (choice > MAX_STUDENTS)
      break;

    choice_to_id[choice - 1] = s.gaia_id();
    auto p = s.StudentAsPerson_person();
    std::cout << choice << ") " << p.FirstName() << " " << p.LastName() << std::endl;
    ++choice;
  }
  }

  while (true)
  {
    auto_transaction_t txn;
    std::string number;
  do {
    std::cout << "Choice: " << std::endl;
    std::cin >> my_choice;

  if (my_choice <= MAX_STUDENTS && my_choice > 0) {
    auto s = student_t::get(choice_to_id[my_choice - 1]);
    auto p = s.StudentAsPerson_person();
    number = s.Number(); 
    
    std:: cout << "To: " << p.FirstName() << " " << p.LastName() << std::endl;
    break;
  }
    std::cout << "Not a choice." << std::endl;
  } while (true);

    msg_number(number, "YO");
  
    txn.commit();
  }

  gaia::system::shutdown();
  return 0;
}
