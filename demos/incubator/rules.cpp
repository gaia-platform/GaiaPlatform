#include "rules.hpp"
#include "barn_storage_gaia_generated.h"
#include "events.hpp"
#include "gaia_system.hpp"

using namespace BarnStorage;
using namespace gaia::rules;

enum MODE { LIST, SHOW, ADD };

const double FAN_SPEED_LIMIT = 3500.0;
int TIMESTAMP = 0;

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
  const table_context_t *t =
      static_cast<const table_context_t *>(context);
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
}

void usage(const char *command) {
  printf("Usage: %s [list|show|add]\n", command);
  printf("\tlist\tlist rules in the system.\n");
  printf("\tshow\tshow available rules to add.\n");
  printf("\tadd\tadd available rules to the system.\n");
}

void list_rules() {
    gaia::rules::list_subscriptions_t subs;
    gaia_type_t gaia_type_filter{kSensorType};
    event_type_t event_type_filter{event_type_t::row_update};
    list_subscribed_rules("Incubator", &gaia_type_filter, &event_type_filter, subs);
    printf("Number of rules for incubator: %ld\n", subs.size());
    for (auto& s : subs) {
        printf("%s\n", s->rule_name);
    }
}

int main(int argc, const char **argv) {
  MODE mode;
  if (argc == 2 && strcmp(argv[1], "list") == 0) {
    mode = LIST;
  } else if (argc == 2 && strcmp(argv[1], "show") == 0) {
    mode = SHOW;
  } else if (argc == 2 && strcmp(argv[1], "add") == 0) {
    mode = ADD;
  } else {
    fprintf(stderr, "Wrong argument.\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (mode == LIST) {
      gaia::system::initialize(false);
      list_rules();
  } else if (mode == ADD) {
      gaia::system::initialize(true);
      gaia_base_t::begin_transaction();
      rule_binding_t fan_control("Incubator", "Fan control", on_sensor_changed);
      subscribe_table_rule(kSensorType, event_type_t::row_update, fan_control);
      gaia_base_t::commit_transaction();
      list_rules();
  }
  return EXIT_SUCCESS;
}
