# Test Description

The `smoke-time-test` test is one of the foundational tests.

## Purpose

The main purpose of this test is to provide an alternative
to the `smoke` test that tries an alternative approach to
waiting for confirmation that all rules have been processed.

## Discussion

This test uses the `t` command to move the simulation
forward by one step, and then waiting for the amount of
microseconds specified by the `T` command.  The `o` command
is used around that command to get a decently accurate
measurement of the time required to execute just that repeated
set of commands in the test's `measured-duration-sec` field.
