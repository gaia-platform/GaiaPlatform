#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to scan memory and provide data that is logged.
"""

import subprocess
import time
import errno
import os
import sys


def __execute_script(command_list):
    with subprocess.Popen(
        command_list,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    ) as process:

        return_code = None
        while return_code is None:
            time.sleep(0.1)
            return_code = process.poll()

        out_lines = process.stdout.readlines()
        error_lines = process.stderr.readlines()
        return return_code, out_lines, error_lines


def __execute_pgrep_for_single_application(grep_pattern):
    command_list = ["pgrep", grep_pattern]
    return_code, output_lines, _ = __execute_script(command_list)

    found_process_id = None
    if return_code == 0:
        assert len(output_lines) == 1
        found_process_id = output_lines[0]
    return found_process_id


def __execute_ps_for_application():
    command_list = ["ps", "-o", "ppid,pid,user,command", "-ax"]
    collected_lines = []
    return_code, output_lines, _ = __execute_script(command_list)
    assert not return_code

    for next_line in output_lines:
        while "  " in next_line:
            next_line = next_line.replace("  ", " ")
        if "build" in next_line:
            split_next_line = next_line.split(" ")
            # ps_command_line = " ".join(split_next_line[3:])
            # f ps_command_line.startswith("./build"):
            # print(">>next_line:" + str(next_line))
            collected_lines.append(split_next_line[1])

    app_process_id = None
    if collected_lines:
        assert len(collected_lines) == 1
        app_process_id = collected_lines[0]
    return app_process_id


def __get_single_sample():
    gaia_db_process_id = __execute_pgrep_for_single_application("gaia_db_server")
    if gaia_db_process_id:
        test_application_process_id = None
        # test_application_process_id = __execute_ps_for_application()
        # print(str(test_application_process_id))
        process_ids = gaia_db_process_id
        if test_application_process_id:
            process_ids += "," + test_application_process_id

        seconds_since_epoch = time.time()
        current_directory = os.path.dirname(os.path.realpath(__file__))
        other_script_path = os.path.join(current_directory, "ps_mem.py")

        return_code, output_lines, _ = __execute_script(
            [other_script_path, "-s", "-k", "-S", "-p", process_ids]
        )
        if return_code in (0, errno.EISDIR):
            for line_index, next_line in enumerate(output_lines):
                if line_index < 2:
                    continue
                if next_line.startswith("---"):
                    break
                next_line = next_line.strip()

                split_main_line = next_line.split("\t")
                i = split_main_line[0]
                while "  " in i:
                    i = i.replace("  ", " ")
                split_statistics = i.split(" ")
                assert split_statistics[1] == "KiB"
                assert split_statistics[4] == "KiB"
                assert split_statistics[7] == "KiB"
                assert split_statistics[9] == "KiB"
                print(
                    f"{int(seconds_since_epoch)},{split_statistics[0]},"
                    + f"{split_statistics[3]},{split_statistics[6]},"
                    + f"{split_statistics[8]},{split_main_line[1]}"
                )
                sys.stdout.flush()
    return gaia_db_process_id


db_process_id = __get_single_sample()
while db_process_id:
    time.sleep(5)
    db_process_id = __get_single_sample()
