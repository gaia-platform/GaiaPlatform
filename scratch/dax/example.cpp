// reference the one I care about, note that this prototype is
// generated

/**
 * assume the following user schema
 * 
 */

/*
    table incubators
    {
        int64_t id; // incubator id, must be unique
        string name;
        float lower_limit;
        float upper_limit;
    };

    table sensors
    {
        int64_t id; // sensor id, must be unique
        string name; // sensor description
        int64_t inc_id; // id of incubator this sensor belongs to
    };

    table sensor_values
    {
        int64_t sensor_id;
        int64_t timestamp;
        float value;
    };
*/


/**
 * insert case with rule on insert give nthe above schema
 * assume fictional hardware device/module that exposes device_read
 * and device_write APIs.
 * 
 */

// user code
void update_sensor(int64_t sensor_id)
{
    // read_sensor value from device
    float value = device_read(sensor_id);
    gaia::begin_transaction();
    sensor_values * v = new sensor_values(sensor_id, time(), value);
    v.insert();
    gaia::commit_transaction();
}

void set_new_limit(const Incubator * inc, float lower_limit, float upper_limit)
{
    gaia::begin_transaction()
    inc->set_lower_limit(lower_limit);
    inc->set_upper_limit(upper_limit);
    inc->update();
    gaia:commit_transaction()
}

// generated insert call based on function binding
// note that this require a transaction context
void sensor_values::insert() {
    if (_copy) {
        // call the event before the insert to get any user changes
        table_event e(this, event_type::insert, gaia_types::sensor_value);
        gaia_api::log_event(e, event_mode::immediate);
        flatbuffers::FlatBufferBuilder b(128);
        auto a_new = sensor_values::Pack(b, _copy);
        b.Finish(a_new);
        gaia_se_node::create(this->Gaia_id(), gaia_types::sensor_value, b.GetSize(), b.GetBufferPointer());        
    }
}

void incubators::set_lower_limit(float limit) {
    copy()->lower_limit = limit;
    field_change_event e(this, gaia_types::incubators);
    gaia_api::log_event(e, event_mode::immediate);
}


// since we bound to value ... 
void sensor_values::set_value(float value) {
    _copy->value = value;
    update_event e(this);
    gaia_api::log_event(e, event_mode::immediate);
}


// user helper code
enum class fan_direction {
    increase,
    decrease
};

enum class sensor_type {
    fan,
    temp
};

float clamp(float value, float min_value, float max_value)
{
    if (value > max_value)
        return max_value;
    
    return (value < min_value) ? min_value : value;
}



// either increments or decrements all the fans for a given incubator
void change_fan_speed(const incubator * inc, fan_direction direction) {
   auto fans = sensors::find("inc_id = {0} AND type = {1}", inc.id, sensor_type::fan);
    // read the last value and then decrement
    for (fan : fans) {
        float last_value = sensor_values::find_top("id = {0} ORDER BY timestamp DESC", fan.id).value;
        float new_value = (direction == fan_direction::increase) ? last_value + FAN_STEP : last_value - FAN_STEP);
        device_write(fan.id, new_value);
        sensor_values * row = new sensor_values(fan.id, time(), new_value);
        row.insert();
    }
}

// lookup incubator that this sensor belongs to
// client could also have used their own index or map
const Incubator * lookup_incubator(uint64_t sensor_id)
{
    const sensors * s = sensors::find("id={0}", sensor_id);
    if (s) {
        return incubators::find("id={0}", s.inc_id)
    }
    return nullptr;
}

sensor_type get_sensor_type(uint64_t sensor_id)
{
    return sensors::find_top("id = {0}", sensor_id).type;
}

void handle_temp_updates(const Sensor_Values * sensor_values)
{
    // ignore updates to fan actuators
    if (get_sensor_type(sensor_values.id) == sensor_type::fan) {
        return;
    }

    // If the temperature didn't change from last time
    // then ignore.  Note that a row is inserted every time the
    // sensor is read so it is expected that it may have the same value
    // over time.
    if (sensor_values.value == sensor_values.get_original_value()) {
        return;
    } 

    // lookup temperature limit of incubator from sensor
    const incubator * inc = lookup_incubator(sensor_values.id)
    if (!inc) {
        return;
    }
    if (sensor_values.value < inc.lower_limit) {
        change_fan_speed(inc, fan_direction::decrease)
    }
    else if (sensor_values.value > inc.upper_limit) {
        change_fan_speed(inc, fan_direction::increase)
    }
}

// rule code with sql processor integration
ruleset temp_control {
    // rule 1:  keep the incubator in the right temperature range
    // different notations.
    //
    // only fire when the column 'value' is changed for an insert
    // or update
    //
    // fire event on sensor_values table for an insert or update if
    // any the value or timestamp column change
    // @event[sensor_values](update value, timestamp)
    // @event[sensor_values](insert)
    //
    
    @event[sensor_values](update value)
    {
        handle_temp_updates(sensor_values)
    }

    @event[sensor_values](insert)
    {
        handle_temp_updates(sensor_values);
    }


    // rule 2:  ensure we don't rail our actuators by clamping to min/max values
    @event[sensor_values](insert)
    @event[sensor_values](update value)
    {
        if (sensor_values.type == sensor_type::fan) {
            sensor_values.value = clamp(sensor_values.value, FAN_MIN_VAL, FAN_MAX_VAL);
        }
    }

    // recompute everything if someone changed the
    // temperature limits on an incubator
    // only care if the limit fields changed
    @event[incubators](update lower_limit, upper_limit)
    {
        // did the lower_limit change?
        if (incubators.lower_limit != incubators.original_lower_limit()){
            // exercise for the reader
        }
        if (incubators.upper_limit != incubators.get_original_upper_limit()){
            // exercise for the reader
        }
    }
    @event(commit)
}

float Incubators::lower_limit(){
    return (_copy ? _copy->lower_limit : _fb->lower_limit());
}

float Incubators::original_lower_limit() {
    return _fb->lower_limit();
}

// rule-2
void rule_2_e68dcb4a1af24ebfa67dff56e9dc434e(const rule_context * c) throw (gaia_api::common::gaia_rule_exception)
{
    void change_context * cc = dynamic_cast<change_context * c>;
    const Incubators * incubators = dynamic_cast<Incubators *>(fc.get_row());
}

// rule generated code
// rule-1
void rule_80064d9107574824b05c7801d2a70494(const RuleContext * c) throw (gaia_api::common::gaia_rule_exception) {
    const insert_context * ic = dynamic_cast<InsertContext *>(c);
    const Sensor_Values * sensor_values = dynamic_cast<Sensor_Values *>(icc.get_row())
    try {
        // ignore updates to actuators
        if (get_sensor_type(sensor_values.id) == sensor_type::fan) {
            return;
        }

        // If the temperature didn't change from last time
        // then ignore.  Note that a row is inserted every time the
        // sensor is read so it is expected that it may have the same value
        // over time.
        if (sensor_values.value == sensor_values.get_original_value()) {
            return;
        } 

        // lookup temperature limit of incubator from sensor
        const incubator * inc = lookup_incubator(c.sensor_values.id)
        if (!inc) {
            return;
        }
        if (sensor_values.value < inc.lower_limit) {
            change_fan_speed(inc, fan_direction::decrease)
        }
        else if (sensor_values.value > inc.upper_limit) {
            change_fan_speed(inc, fan_direction::increase)
        }
    }
    catch(std::exception& e){
        string message = "rule_80064d9107574824b05c7801d2a70494";
        message.append(e.what)
        throw gaia_rule_exception(message);
    }
}

//
// actual call sequence of above given the following gaia/deb objects:
// sensor_values :  table that contains a new value and timestamp for a sensor.  New sensor values are added
//  by inserting into this table
// sensors:  all the sensors/actuators in the system
// incubators:  all the incubators in the system.
//
//
/*
1) user reads a new temperature value from the sensor
2) user writes the value to sensor_values
3) this triggers two events above (rule1 and rule2), order is not determined
4) say rule2 fires - sensor_values is the *new* row that is about to be inserted but the type in sensor_type::temp so nothing happens
5a) rule1 fires - if the  temp is in range of the incubator limits, nothing happens, control returns to user code (temp_sensor::update())
5b) rule1 fires, temp is < limit, fan value is decreased and a new row is inserted
6) rule1 fires again this time for the new fan value.  Since the sensor type is a fan we immediately return
7) rule2 fires with fan sensor value.  Since it is sensor_type::fan, we run the clamp code to update the value before insert
8) fan value is written to sensor_values table
9) temperature value is written to the sensor_values table.  This ordering is kind of wonky.
*/

//
// support/conventions needed
//
/*
1) gaia generated api surfaces static insert/delete methods on the user's flatbuffer type so taht you can do user_type::<insert | delete>
2) sql processing is exposed as a gaia api.  Syntax is the SQL syntax for WHERE clauses and ORDER BY.  The ::find* methods are generated
    per type as a convenience method
3) find return an iterable type (std::list, vector, etc) of the type of the flatbuffer type.
4) find_top returns a single item
5) query processor must be able to understand flatbuffer schema so that the field names can be identified
6) in this example, user setup the schema to relate the primary and foreign keys.  (incubator id, sensor id, etc)
7) user rule's can invoke arbitrary user code
8) user code cannot invoke user rules
9) event bindings are made explicit by @event.  
10) BNF is @event[<table name>]([insert], [update], [delete]).
11) if no table is specified then the scope is assumed to be the database.  So syntax is @event([create], [drop], [alter])
if we only ever have one database then we should be able to distinguish whether the name is a table or database.
12) unclear what the context would be for a commit event.  It's omitted at this time.  Although we'll want the event manager to execute
all pending events at commit time, it's unclear that a user would bind a rule to a commit event.
3) 
*/

//
// TODO: the whole point of this exercise is to figure out the user code -> event -> user rule context bindings
// TODO: can possibly solve the relation with find/find_top (or at least a stepping stone)
// TODO: document "TRIGGER" syntax for events
// TODO: document WHERE syntax
//
1) event declaration tells us what table the event is on.  If defined unambiguously, it will also tell us the scope.
2) for update, the context is the row of the table that was updated
3) for insert/delete, the context is the row that is about to be inserted or deleted

//
// Context sent when the event scope is table
// add these to gaia_direct.h
//

using namespace gaia::api;

class context {
public:    
    event_type type;
};

// rule_context contains 

class table_context: public context
{
    const gaia_base * get_row() { return _}
public:

    const gaia_base * row; // flatbuffer type of the row that was changed
};


// tag class
class rule_context
{
    virtual ~rule_context() = default;
    bool result;
};

class insert_context : public rule_context
{
    const gaia_base * get_row() { return _e.row; }
private:
    const insert_event& _e;    
};

class update_context : public rule_context
{
public:
    const gaia_base * get_row() {}

private:
    const update_event& _e 
};

private:
    shared_ptr<context> context;
};

class table_update_context: public table_context
{
public:
    // array of field locators to rows that changed?
    // how do I use these to get new and original values?
    std::vector<FlatBuffersVTableOffset> changed;
    // then user can just compare to VT_AP_ID, VT_IATA, etc


    // if we go this route, this should be the VT_FLD_ID
    // we can get types and names using min_reflection from flatbuffers
    // but I'm not sure if this is useful/needed or not
    /*
    using would look like, how do I know what field changed though?
    we don't have a name per se...

    value = sensor_values.locate(changed[0]);
    old_value = old_sensor_values.locate(changed[0]); // right, and what type would that return?  Need <T>

    // nicer syntax would be somthing like
    // below but only if strongly typed (i.e., non-variant)
    field_locator * fld = changed[0];
    value = fld.value;
    old_value = fld.old_value;
    // furthermore
    fld.value = new_value; should stick if written to by the user's rule.
    */
};

// should just be the flatbuffer offset
    
class GaiaTableUpdateContext: public GaiaContext


// should be able to get old values from a row
// no do this on a per-field basis so it's typed
const sensor_values * old_values = sensor_values->get_original_values();

TODO:// write up document that shows
- flow above (maybe another sequence diagram)
- proposed event syntax in rules BNF
- context classes
- flow of context from user code -> pending event -> user rule


