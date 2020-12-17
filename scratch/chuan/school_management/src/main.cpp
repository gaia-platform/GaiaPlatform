#include "gaia/db/db.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "pistache/http.h"
#include "pistache/router.h"

#include <signal.h>
#include <unistd.h>
#include <vector>

#include "EventApiImpl.h"
#include "FacilityApiImpl.h"
#include "PersonnelApiImpl.h"

#define PISTACHE_SERVER_THREADS 2
#define PISTACHE_SERVER_MAX_REQUEST_SIZE 32768
#define PISTACHE_SERVER_MAX_RESPONSE_SIZE 32768

static Pistache::Http::Endpoint *httpEndpoint;
static void sigHandler [[noreturn]] (int sig) {
  switch (sig) {
  case SIGINT:
  case SIGQUIT:
  case SIGTERM:
  case SIGHUP:
  default:
    httpEndpoint->shutdown();
    break;
  }
  exit(0);
}

static void setUpUnixSignals(std::vector<int> quitSignals) {
  sigset_t blocking_mask;
  sigemptyset(&blocking_mask);
  for (auto sig : quitSignals)
    sigaddset(&blocking_mask, sig);

  struct sigaction sa;
  sa.sa_handler = sigHandler;
  sa.sa_mask = blocking_mask;
  sa.sa_flags = 0;

  for (auto sig : quitSignals)
    sigaction(sig, &sa, nullptr);
}

using namespace org::openapitools::server::api;

using namespace gaia::rules;
using namespace gaia::db::triggers;

void list_rules() {
  subscription_list_t subs;
  const char *subscription_format = "%-18s|%-30s|%5s|%-10s|%5s\n";
  list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subs);
  printf("Number of rules: %ld\n", subs.size());
  if (subs.size() > 0) {
    printf("\n");
    printf(subscription_format, "ruleset", "rule", "type", "event", "field");
    printf("-------------------------------------------------------------------"
           "------\n");
  }
  std::map<event_type_t, const char *> event_names;
  event_names[event_type_t::row_update] = "Row update";
  event_names[event_type_t::row_insert] = "Row insert";
  for (auto &s : subs) {
    printf(subscription_format, s->ruleset_name, s->rule_name,
           std::to_string(s->gaia_type).c_str(), event_names[s->event_type],
           std::to_string(s->field).c_str());
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  gaia::system::initialize("./gaia.conf");
  list_rules();

  std::vector<int> sigs{SIGQUIT, SIGINT, SIGTERM, SIGHUP};
  setUpUnixSignals(sigs);
  Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8080));

  httpEndpoint = new Pistache::Http::Endpoint((addr));
  auto router = std::make_shared<Pistache::Rest::Router>();

  auto opts =
      Pistache::Http::Endpoint::options().threads(PISTACHE_SERVER_THREADS);
  opts.flags(Pistache::Tcp::Options::ReuseAddr);
  opts.maxRequestSize(PISTACHE_SERVER_MAX_REQUEST_SIZE);
  opts.maxResponseSize(PISTACHE_SERVER_MAX_RESPONSE_SIZE);
  httpEndpoint->init(opts);

  EventApiImpl EventApiserver(router);
  EventApiserver.init();
  FacilityApiImpl FacilityApiserver(router);
  FacilityApiserver.init();
  PersonnelApiImpl PersonnelApiserver(router);
  PersonnelApiserver.init();

  httpEndpoint->setHandler(router->handler());
  httpEndpoint->serve();

  httpEndpoint->shutdown();

  gaia::system::shutdown();

  return 0;
}
