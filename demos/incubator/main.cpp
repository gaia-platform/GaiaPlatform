#include "barn_storage_gaia_generated.h"
#include "events.hpp"
#include "gaia_system.hpp"
#include "rules.hpp"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <iostream>

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace BarnStorage;

enum { chicken_incubator_id = 1, puppy_incubator_id = 2 };

int TIMESTAMP = 0;
const double FAN_SPEED_LIMIT = 3500.0;
const char SENSOR_A_NAME[] = "Temp A";
const char SENSOR_B_NAME[] = "Temp B";
const char SENSOR_C_NAME[] = "Temp C";
const char ACTUATOR_A_NAME[] = "Fan A";
const char ACTUATOR_B_NAME[] = "Fan B";
const char ACTUATOR_C_NAME[] = "Fan C";

void init_storage() {
  gaia_base_t::begin_transaction();

  Incubator::insert_row(chicken_incubator_id, "Chicken", 99.0, 102.0);
  Sensor::insert_row(chicken_incubator_id, SENSOR_A_NAME, 0, 99.0);
  Sensor::insert_row(chicken_incubator_id, SENSOR_C_NAME, 0, 99.0);
  Actuator::insert_row(chicken_incubator_id, ACTUATOR_A_NAME, 0, 0.0);

  Incubator::insert_row(puppy_incubator_id, "Puppy", 85.0, 90.0);
  Sensor::insert_row(puppy_incubator_id, SENSOR_B_NAME, 0, 85.0);
  Actuator::insert_row(puppy_incubator_id, ACTUATOR_B_NAME, 0, 0.0);
  Actuator::insert_row(puppy_incubator_id, ACTUATOR_C_NAME, 0, 0.0);

  gaia_base_t::commit_transaction();
}

void dump_db() {
  gaia_base_t::begin_transaction();
  printf("\n");
  printf("> SELECT * FROM incubator, sensor WHERE incubator.id = "
         "sensor.incubator_id\n");
  printf("  UNION\n");
  printf("  SELECT * FROM incubator, actuator WHERE incubator.id = "
         "actuator.incubator_id;\n\n");

  printf("  name  | min_temp | max_temp | name | time | value \n");
  printf("------------------------------------------------------\n");
  for (auto i = Incubator::get_first(); i != nullptr; i = i->get_next()) {
    for (auto s = Sensor::get_first(); s != nullptr; s = s->get_next()) {
      if (s->incubator_id() == i->id()) {
        printf("%8s|%10.1lf|%10.1lf|%7s|%6ld|%7.1lf\n", i->name(),
               i->min_temp(), i->max_temp(), s->name(), s->timestamp(),
               s->value());
      }
    }
    for (auto a = Actuator::get_first(); a != nullptr; a = a->get_next()) {
      if (a->incubator_id() == i->id()) {
        printf("%8s|%10.1lf|%10.1lf|%7s|%6ld|%7.1lf\n", i->name(),
               i->min_temp(), i->max_temp(), a->name(), a->timestamp(),
               a->value());
      }
    }
  }
  gaia_base_t::commit_transaction();
}

double calc_new_temp(float curr_temp, float fan_speed) {
  if (fan_speed < 1000) {
    return curr_temp + 0.1;
  } else if (fan_speed < 3000) {
    return curr_temp - 0.1;
  }
  return curr_temp - 0.2;
}

void simulation() {
  time_t start, cur;
  time(&start);
  while (true) {
    sleep(1);
    gaia_base_t::begin_transaction();
    double new_temp, fa_v, fb_v, fc_v;
    for (auto a = Actuator::get_first(); a != nullptr; a = a->get_next()) {
      if (strcmp(a->name(), ACTUATOR_A_NAME) == 0) {
        fa_v = a->value();
      } else if (strcmp(a->name(), ACTUATOR_B_NAME) == 0) {
        fb_v = a->value();
      } else if (strcmp(a->name(), ACTUATOR_C_NAME) == 0) {
        fc_v = a->value();
      }
    }
    time(&cur);
    TIMESTAMP = difftime(cur, start);
    for (auto s = Sensor::get_first(); s != nullptr; s = s->get_next()) {
      if (strcmp(s->name(), SENSOR_A_NAME) == 0) {
        new_temp = calc_new_temp(s->value(), fa_v);
        s->set_value(new_temp);
        s->set_timestamp(TIMESTAMP);
        s->update_row();
      } else if (strcmp(s->name(), SENSOR_B_NAME) == 0) {
        new_temp = calc_new_temp(s->value(), fb_v);
        new_temp = calc_new_temp(new_temp, fc_v);
        s->set_value(new_temp);
        s->set_timestamp(TIMESTAMP);
        s->update_row();
      } else if (strcmp(s->name(), SENSOR_C_NAME) == 0) {
        new_temp = calc_new_temp(s->value(), fa_v);
        s->set_value(new_temp);
        s->set_timestamp(TIMESTAMP);
        s->update_row();
      }
    }
    gaia_base_t::commit_transaction();
  }
}

Incubator *select_incubator_by_id(ulong id) {
  for (auto i = Incubator::get_first(); i != nullptr; i = i->get_next()) {
    if (i->id() == id) {
      return i;
    }
  }
  return nullptr;
}

void decrease_fans(Incubator *incubator) {
  printf("%s called for %s incubator.\n", __func__, incubator->name());
  for (auto a = Actuator::get_first(); a != nullptr; a = a->get_next()) {
    if (a->incubator_id() == incubator->id()) {
      a->set_value(max(0.0, a->value() - 500.0));
      a->set_timestamp(TIMESTAMP);
      a->update_row();
    }
  }
}

void increase_fans(Incubator *incubator) {
  printf("%s called for %s incubator.\n", __func__, incubator->name());
  for (auto a = Actuator::get_first(); a != nullptr; a = a->get_next()) {
    if (a->incubator_id() == incubator->id()) {
      a->set_value(min(FAN_SPEED_LIMIT, a->value() + 500.0));
      a->set_timestamp(TIMESTAMP);
      a->update_row();
    }
  }
}

void on_sensor_changed(const context_base_t *context) {
  const table_context_t *t = static_cast<const table_context_t *>(context);
  Sensor *s = static_cast<Sensor *>(t->row);
  Incubator *i = select_incubator_by_id(s->incubator_id());
  printf("%s fired for %s sensor of %s incubator\n", __func__, s->name(),
         i->name());

  double cur_temp = s->value();
  if (cur_temp < i->min_temp()) {
    decrease_fans(i);
  } else if (cur_temp > i->max_temp()) {
    increase_fans(i);
  }
}

extern "C" void initialize_rules() {
  rule_binding_t fan_control("Incubator", "Fan control", on_sensor_changed);
  subscribe_table_rule(kSensorType, event_type_t::row_update, fan_control);
}

int main(int argc, const char **argv) {
  bool is_sim = false;

  if (argc == 2 && strncmp(argv[1], "sim", 3) == 0) {
    is_sim = true;
  } else if (argc != 1) {
    cout << "Wrong arguments." << endl;
    return EXIT_FAILURE;
  }

  if (!is_sim) {
    gaia_mem_base::init(false);
    dump_db();
    return EXIT_SUCCESS;
  }

  gaia::system::initialize(true);
  init_storage();
  dump_db();
  simulation();
  return EXIT_SUCCESS;
}
