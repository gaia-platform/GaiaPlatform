create table event_log (
    event_type: uint32,
    type_id: uint64,
    record_id: uint64,
    column_id: uint16,
    timestamp: uint64,
    rules_invoked: bool
);
