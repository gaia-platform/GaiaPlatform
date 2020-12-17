#include "phone.h"
#include "gaia/system.hpp"
#include "gaia/rules/rules.hpp"

#include <iostream>
#include <unistd.h>

using namespace gaia::school;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;

constexpr size_t MAX_STUDENTS = 16;

int main() {
  // select phone
  gaia::system::initialize();

  std::cout << "Magical messaging system" << std::endl;
  std::cout << "Who is this? " << std::endl;

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

  {
    auto_transaction_t txn;
  do {
    std::cout << "Choice: " << std::endl;
    std::cin >> my_choice;

  if (my_choice <= MAX_STUDENTS && my_choice > 0) {
    auto s = student_t::get(choice_to_id[my_choice - 1]);
    auto p = s.StudentAsPerson_person();
    std:: cout << "Hello " << p.FirstName() << " " << p.LastName() << std::endl;
    set_my_phone(s.Number());
    break;
  }

    std::cout << "Not a choice." << std::endl;
  } while (true);
  }

  std::cout << "Your number " << my_phone() << std::endl;
  while (true) {
    sleep(500);
  };

  return 0;
}
