#include "trigger.hpp"
#include "event_manager.hpp"
#include <vector>
#include <memory>

using namespace gaia::db::triggers;

void event_trigger::commit_trigger_(int64_t tx_id,
                                shared_ptr<std::vector<unique_ptr<triggers::trigger_event_t>>> events,
                                size_t count_events,
                                bool immediate) {
    
    gaia::rules::event_manager_t::get().commit_trigger_se(tx_id, events, count_events, immediate);
    events->clear();
}

