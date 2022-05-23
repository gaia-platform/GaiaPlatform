----------------------------------------------------
-- Copyright (c) Gaia Platform LLC
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

database amr_swarm

--
-- "Constant" objects
--

table station_type (
    -- Fields
    id                                  uint8  UNIQUE,
    sandbox_id                          string,
    pallet_capacity                     uint32,
    widget_capacity                     uint32,

    -- References
    stations                            references station[]
                                            where station.station_type_id = station_type.id
)

-- Robot types. There are 2 types: robots that carry pallets and robots that carry widgets.
table robot_type (
    -- Fields
    id                                  uint8 UNIQUE,
    sandbox_id                          string,
    pallet_capacity                     uint32,
    widget_capacity                     uint32,

    -- References
    robots                              references robot[]
                                            where robot.robot_type_id = robot_type.id
)

table configuration (
    -- Fields
    widget_bot_recharge_threshold       double,
    pallet_bot_recharge_threshold       double,
    number_of_pallet_bots               uint8,
    number_of_widget_bots               uint8,

    -- References
    robots                              references robot[] using config,
    main_pallet_bot                     references robot using main_pallet_botx,
    left_widget_bot                     references robot using left_widget_botx,
    right_widget_bot                    references robot using right_widget_botx
)

--
-- Simulation objects
--

table station (
    -- Fields
    id                                  uint16 UNIQUE,
    sandbox_id                          string,

    -- Parent References
    robots                              references robot[]
                                            where robot.station_id = station.id,
    pallets                             references pallet[]
                                            where pallet.station_id = station.id,
    widgets                             references widget[]
                                            where widget.station_id = station.id,

    -- Child References
    type                                references station_type,
    station_type_id                     uint8,

    -- Events
    bot_moving_to_station_events        references bot_moving_to_station_event[]
                                            where bot_moving_to_station_event.station_id = station.id,
    bot_arrived_events                  references bot_arrived_event[]
                                            where bot_arrived_event.station_id = station.id,

    -- Child Events
    bot_arrived_at_charging_station_events  references bot_arrived_at_charging_station_event[]
                                                where bot_arrived_at_charging_station_event.station_id = station.id,
    bot_arrived_at_inbound_station_events   references bot_arrived_at_inbound_station_event[]
                                                where bot_arrived_at_inbound_station_event.station_id = station.id,
    bot_arrived_at_buffer_station_events    references bot_arrived_at_buffer_station_event[]
                                                where bot_arrived_at_buffer_station_event.station_id = station.id,
    bot_arrived_at_pl_start_station_events  references bot_arrived_at_pl_start_station_event[]
                                                where bot_arrived_at_pl_start_station_event.station_id = station.id,
    bot_arrived_at_pl_end_station_events    references bot_arrived_at_pl_end_station_event[]
                                                where bot_arrived_at_pl_end_station_event.station_id = station.id,
    bot_arrived_at_outbound_station_events  references bot_arrived_at_outbound_station_event[]
                                                where bot_arrived_at_outbound_station_event.station_id = station.id,

    -- Tasks
    check_buffer_start_tasks            references check_buffer_start_task[]
                                            where check_buffer_start_task.station_id = station.id,
    check_production_start_tasks        references check_production_start_task[]
                                            where check_production_start_task.station_id = station.id
)

-- Robots. A pallet robot can carry 1 pallet. A widget robot can carry 1 widget.
table robot (
    -- Fields
    id                                  uint16 UNIQUE,
    sandbox_id                          string,
    charge                              float,
    is_stuck                            bool,
    is_idle                             bool,
    is_loaded                           bool,

    -- References
    config                              references configuration,
    main_pallet_botx                    references configuration using main_pallet_bot,
    left_widget_botx                    references configuration using left_widget_bot,
    right_widget_botx                   references configuration using right_widget_bot,

    -- Parent References
    pallets                             references pallet[]
                                            where pallet.robot_id = robot.id,
    widgets                             references widget[]
                                            where widget.robot_id = robot.id,

    -- Child References
    type                                references robot_type,
    robot_type_id                       uint8,
    station                             references station,
    station_id                          uint16,

    -- Events
    bot_arrived_events                  references bot_arrived_event[]
                                            where bot_arrived_event.robot_id = robot.id,
    bot_moving_to_station_events        references bot_moving_to_station_event[]
                                            where bot_moving_to_station_event.robot_id = robot.id,
    bot_picked_up_payload_events        references bot_picked_up_payload_event[]
                                            where bot_picked_up_payload_event.robot_id = robot.id,
    bot_dropped_off_payload_events      references bot_dropped_off_payload_event[]
                                            where bot_dropped_off_payload_event.robot_id = robot.id,
    bot_cannot_navigate_events          references bot_cannot_navigate_event[]
                                            where bot_cannot_navigate_event.robot_id = robot.id,
    bot_crashed_events                  references bot_crashed_event[]
                                            where bot_crashed_event.robot_id = robot.id,
    bot_charging_events                 references bot_charging_event[]
                                            where bot_charging_event.robot_id = robot.id,
    bot_is_charged_events               references bot_is_charged_event[]
                                            where bot_is_charged_event.robot_id = robot.id,
    bot_battery_charge_update_events    references bot_battery_charge_update_event[]
                                            where bot_battery_charge_update_event.robot_id = robot.id,
    bot_out_of_battery_events           references bot_out_of_battery_event[]
                                            where bot_out_of_battery_event.robot_id = robot.id,

    -- Child Events
    bot_arrived_at_charging_station_events  references bot_arrived_at_charging_station_event[]
                                                where bot_arrived_at_charging_station_event.robot_id = robot.id,
    bot_arrived_at_inbound_station_events   references bot_arrived_at_inbound_station_event[]
                                                where bot_arrived_at_inbound_station_event.robot_id = robot.id,
    bot_arrived_at_buffer_station_events    references bot_arrived_at_buffer_station_event[]
                                                where bot_arrived_at_buffer_station_event.robot_id = robot.id,
    bot_arrived_at_pl_start_station_events  references bot_arrived_at_pl_start_station_event[]
                                                where bot_arrived_at_pl_start_station_event.robot_id = robot.id,
    bot_arrived_at_pl_end_station_events    references bot_arrived_at_pl_end_station_event[]
                                                where bot_arrived_at_pl_end_station_event.robot_id = robot.id,
    bot_arrived_at_outbound_station_events  references bot_arrived_at_outbound_station_event[]
                                                where bot_arrived_at_outbound_station_event.robot_id = robot.id
)

table pallet (
    -- Fields
    id                                  string UNIQUE,
    weight                              uint32,
    max_widgets                         uint8,

    -- Parent References
    widgets                             references widget[]
                                            where widget.pallet_id = pallet.id,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16,

    -- Events
    pallet_arrived_events               references pallet_arrived_event[]
                                            where pallet_arrived_event.pallet_id = pallet.id,
    pallet_shipped_events               references pallet_shipped_event[]
                                            where pallet_shipped_event.pallet_id = pallet.id,
    pallet_unpacked_events              references pallet_unpacked_event[]
                                            where pallet_unpacked_event.pallet_id = pallet.id
)

-- Widgets represent raw materials as well as the processed product.
table widget (
    -- Fields
    id                                  string UNIQUE,
    weight                              uint32,

    -- References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16,
    pallet                              references pallet,
    pallet_id                           string,

    -- Events
    widget_ready_for_production_line_event      references widget_ready_for_production_line_event[]
                                                    where widget_ready_for_production_line_event.widget_id = widget.id,
    widget_unloaded_from_production_line_event  references widget_unloaded_from_production_line_event[]
                                                    where widget_unloaded_from_production_line_event.widget_id = widget.id,
    widget_production_finished_event            references widget_production_finished_event[]
                                                    where widget_production_finished_event.widget_id = widget.id
)

--
-- Helper objects
--

table delayed_event (
    -- Fields
    event_type                          uint8,
    event_requeued                      bool,
    delayed_id                          uint64
)

--
-- General Events
--

-- Notify that the setup has started.
table setup_started_event (
    -- Fields
    timestamp                           uint64
)

-- Notify that setup has completed.
table setup_complete_event (
    -- Fields
    timestamp                           uint64
)

--
-- Robot Events
--

-- Notify when a robot has arrived to a station
table bot_arrived_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_arrived_at_charging_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_arrived_at_inbound_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_arrived_at_buffer_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_arrived_at_pl_start_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_arrived_at_pl_end_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_arrived_at_outbound_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

table bot_moving_to_station_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16,
    robot                               references robot,
    robot_id                            uint16
)

-- Notify the robot has picked up a payload.
table bot_picked_up_payload_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

-- Notify the robot has dropped a payload.
table bot_dropped_off_payload_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

-- Response from bot when it can't complete a navigation because
-- either the path couldn't be generated or if the end location
-- was blocked.
table bot_cannot_navigate_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

-- Reports a bot with ID crashed while going to goal location payload.
table bot_crashed_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

-- Response from bot when it moves into the charging station.
table bot_charging_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

table bot_is_charged_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

table bot_battery_charge_update_event (
    -- Fields
    timestamp                           uint64,
    charge                              float,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

table bot_out_of_battery_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    robot                               references robot,
    robot_id                            uint16
)

--
-- Pallet Events
--

-- Notify when a pallet has arrived to a station
table pallet_arrived_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    pallet                              references pallet,
    pallet_id                           string
)

-- When a pallet is unpacked into the buffer area.
table pallet_unpacked_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    pallet                              references pallet,
    pallet_id                           string
)

-- When a pallet is shipped to an external customer.
table pallet_shipped_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    pallet                              references pallet,
    pallet_id                           string
)

--
-- Widget Events
--

-- When a widget makes it to PL End.
table widget_unloaded_from_production_line_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    widget                              references widget,
    widget_id                           string
)

-- When a widget is dropped of at PL Start and it is ready to be processed.
table widget_ready_for_production_line_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    widget                              references widget,
    widget_id                           string
)

-- When a widget is created and deposited at PL END.
table widget_production_finished_event (
    -- Fields
    timestamp                           uint64,

    -- Child References
    widget                              references widget,
    widget_id                           string
)

--
-- Tasks
--

table requeue_task (
    -- Fields
    timestamp                           uint64,
    delayed_event_type                  uint8
)

table check_buffer_start_task (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16
)

table check_production_start_task (
    -- Fields
    timestamp                           uint64,

    -- Child References
    station                             references station,
    station_id                          uint16
)

table move_pallet_bot_to_inbound_task (
    -- Fields
    timestamp                           uint64
)

table move_widget_bot_to_line_end_task (
    -- Fields
    timestamp                           uint64
)

table move_widget_bot_to_buffer_task (
    -- Fields
    timestamp                           uint64
)

table move_widget_bot_to_outbound_task (
    -- Fields
    timestamp                           uint64
)

table check_for_robot_starts_task (
    -- Fields
    timestamp                           uint64
)

table check_for_outbound_ship_task (
    -- Fields
    timestamp                           uint64
)
