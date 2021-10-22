---------------------------------------------
-- Copyright (c) Gaia Platform LLC
-- All rights reserved.
---------------------------------------------

drop database if exists palletbox;

create database if not exists palletbox;

use palletbox;

table station (
    -- Fields
    id uint16 unique,

    -- References
    robots
        references robot[],

    -- Events
    bot_moving_to_station_events
        references bot_moving_to_station_event[],
    bot_arrived_events
        references bot_arrived_event[],
    payload_pick_up_events
        references payload_pick_up_event[],
    payload_drop_off_events
        references payload_drop_off_event[]
)

table robot (
    -- Fields
    id uint16 unique,
    times_to_charging int16,
    target_times_to_charge int16,

    -- References
    station
        references station
            where robot.station_id = station.id,
    station_id uint16,

    -- Events
    bot_moving_to_station_events
        references bot_moving_to_station_event[],
    bot_arrived_events
        references bot_arrived_event[],
    payload_pick_up_events
        references payload_pick_up_event[],
    payload_drop_off_events
        references payload_drop_off_event[]
)

table bot_moving_to_station_event (
    -- Fields
    timestamp uint64,

    -- References
    station
        references station
            where bot_moving_to_station_event.station_id = station.id,
    station_id uint16,
    robot
        references robot
            where bot_moving_to_station_event.robot_id = robot.id,
    robot_id uint16
)

table bot_arrived_event (
    -- Fields
    timestamp uint64,

    -- References
    station
        references station
            where bot_arrived_event.station_id = station.id,
    station_id uint16,
    robot
        references robot
            where bot_arrived_event.robot_id = robot.id,
    robot_id uint16
)

table payload_pick_up_event (
    -- Fields
    timestamp uint64,

    -- References
    station
        references station
            where payload_pick_up_event.station_id = station.id,
    station_id uint16,
    robot
        references robot
            where payload_pick_up_event.robot_id = robot.id,
    robot_id uint16
)

table payload_drop_off_event (
    -- Fields
    timestamp uint64,

    -- References
    station
        references station
            where payload_drop_off_event.station_id = station.id,
    station_id uint16,
    robot
        references robot
            where payload_drop_off_event.robot_id = robot.id,
    robot_id uint16
)
