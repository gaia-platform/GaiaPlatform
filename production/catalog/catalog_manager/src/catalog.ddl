create table gaia_table (
  name string,
  is_log bool,
  trim_action uint8,
  max_rows uint64,
  max_size uint64,
  max_seconds uint64,
  binary_schema string
);

create table gaia_value_index (
  name string,
  table_id uint64,
  fields string,
  index_type uint8,
  unique bool,
  values_ references gaia_table
);

create table gaia_field (
  name string,
  table_id uint64,
  type uint8,
  type_id uint64,
  repeated_count uint16,
  position uint16,
  required bool,
  deprecated bool,
  active bool,
  nullable bool,
  has_default bool,
  default_value string,
  value_fields_ references gaia_value_index,
  fields_ references gaia_table,
  refs_ references gaia_table
);

create table gaia_ruleset (
  name string,
  active_on_startup bool,
  table_ids string,
  source_location string,
  serial_stream string
);

create table gaia_rule (
  name string,
  ruleset_id uint64,
  rules_ references gaia_ruleset
);
