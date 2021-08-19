# Test Description

The `smoke-long-stats` test is an experimental test.

## Purpose

The main purpose of this test is to provide for a simple
fixture to explore changes in the duration between samples
for the Rules Engine's `gaia_stats.log` file.

## Discussion

There have been discussions regarding the sampling size
used for the `gaia_stats.log` file.  To abet these
concerns, this experimental test increases the sample
time in the `config.json` file to `15` seconds.

This test should not be used in any gatekeeping as
it truly is an experimental test.
