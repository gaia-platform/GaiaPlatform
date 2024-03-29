----------------------------------------------------
-- Copyright (c) Gaia Platform Authors
--
-- Use of this source code is governed by the MIT
-- license that can be found in the LICENSE.txt file
-- or at https://opensource.org/licenses/MIT.
----------------------------------------------------

create database if not exists pingpong;

use pingpong;

create table if not exists pong_table (
      name string,
      rule_timestamp uint64
);

create table if not exists ping_table (
      name string,
      step_timestamp uint64
);

create table if not exists marco_table (
      name string,
      marco_count uint64,
      marco_limit uint64
);

create table if not exists polo_table (
      name string,
      polo_count uint64
);
