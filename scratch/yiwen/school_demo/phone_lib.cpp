#include "phone.h"

std::string g_my_phone;

const std::string& my_phone() {
  return g_my_phone;
}

void set_my_phone(const std::string& number) {
  g_my_phone = number;
}
