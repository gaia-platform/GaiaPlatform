---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

database amr_swarm

-- Type of station. Not used much currently.
table station_type (
    id string,
    pallet_capacity uint32,
    widget_capacity uint32,
    stations references station[]
)

-- Stations.
-- Only one robot can be at a station.
-- Exception: Charging station can hold multiple robots.
table station (
    id string,
    type references station_type,
    robots references robot[],
    pallets references pallet[],
    widgets references widget[],
    bot_moving_to_station_events references bot_moving_to_station_event[],
    bot_arrived_at_station_events references bot_arrived_at_station_event[]
)

-- Robot types. There are 2 types: robots that carry pallets and robots that carry widgets.
table robot_type (
    id string,
    pallet_capacity uint32,
    widget_capacity uint32,
    robots references robot[]
)

-- Robots. A pallet robot can carry 1 pallet. A widget robot can carry 1 widget.
table robot (
    id string,
    charge uint8,
    is_stuck bool,
    is_idle bool,
    is_loaded bool,
    type references robot_type,
    station references station,
    pallets references pallet[],
    widgets references widget[],
    bot_moving_to_station_events references bot_moving_to_station_event[],
    bot_arrived_at_station_events references bot_arrived_at_station_event[],
    bot_pick_up_payload_events references bot_pick_up_payload_event[],
    bot_drop_off_payload_events references bot_drop_off_payload_event[],
    bot_charging_events references bot_charging_event[],
    bot_cant_navigate_events references bot_cant_navigate_event[],
    bot_crashed_events references bot_crashed_event[]
)

-- Pallets hold up to 4 widgets.
table pallet (
    id string,
    weight uint32,
    robot references robot,
    station references station,
    widgets references widget[],
    pallet_arrived_events references pallet_arrived_event[],
    pallet_unpacked_event references pallet_unpacked_event
)

-- Widgets represent raw materials as well as the processed product.
table widget (
    id string,
    weight uint32,
    pallet references pallet,
    robot references robot,
    station references station,
    widget_production_start_event references widget_production_start_event,
    widget_production_end_event references widget_production_end_event,
    widget_processed_event references widget_processed_event
)

-- Heartbeat event.
table ping_event (
    timestamp uint64
)

-- Event generated when pressing the "Receive Order" UI button.
-- Note: the handling code will automatically create
-- the connection between the pallet and the station.
table pallet_arrived_event (
    timestamp uint64,
    pallet references pallet
)

-- Event generated when pressing the "UNPACK!" UI button.
table pallet_unpacked_event (
    timestamp uint64,
    pallet references pallet
)

-- Event generated when calling move_robot() and the move can be performed.
table bot_moving_to_station_event (
    timestamp uint64,
    robot references robot,
    station references station
)

-- Event generated when a robot arrives at a station.
table bot_arrived_at_station_event (
    timestamp uint64,
    robot references robot,
    station references station
)

-- Event generated when calling pickup_pallet() or pickup_widget() and the action can be performed.
-- I.e. the event will not be triggered if there is nothing to pickup.
table bot_pick_up_payload_event (
    timestamp uint64,
    robot references robot
)

-- Event generated when calling drop_pallet() or drop_widget() and the action can be performed.
table bot_drop_off_payload_event (
    timestamp uint64,
    robot references robot
)

-- Event generated when calling charge() and the action can be performed.
table bot_charging_event (
    timestamp uint64,
    robot references robot
)

-- Event generated when calling move_robot()
-- and the path to the destination is unreachable.
table bot_cant_navigate_event (
    timestamp uint64,
    robot references robot
)

-- Event generated when robot crashes into another robot during movement.
table bot_crashed_event (
    timestamp uint64,
    robot references robot
)

-- Event generated when a widget is dropped off at PL Start and it is ready to be processed.
table widget_production_start_event (
    timestamp uint64,
    widget references widget
)

-- Event generated when a widget is created by PL.
table widget_production_end_event (
    timestamp uint64,
    widget references widget
)

-- Event generated when a widget makes it to PL End.
table widget_processed_event (
    timestamp uint64,
    widget references widget
)
