# Test Description

The `smoke` test is one of the foundational tests.

## Purpose

The main purpose of this test is to provide a simple and
efficient manner to test that an installed Gaia system is
up and running properly.

## Discussion

This test mainly uses the `z` command to move the simulation
forward by one step, waiting for the actions of that step
to complete executing, and then output the current state of
the data store.  The `o` command is used around that command
to get a decently accurate measurement of the time required
to execute just that repeated set of commands in the test's
`measured-duration-sec` field.
