#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide high level scenario tests for the Docker_Dev project.
"""

from test.gdev_execute import (
    determine_old_script_behavior,
    determine_repository_production_directory,
    SubprocessExecutor,
    InProcessResult,
)
import io

HELP_POSITIONAL_PREFIX = """positional arguments:
  args                  Zero or more arguments to be forwarded on to docker
                        run. If one or more arguments are provided, the first
                        argument must be `--`.

"""
HELP_BASE_ARGUMENTS = """optional arguments:
  -h, --help            show this help message and exit
  --log-level {CRITICAL,ERROR,WARNING,INFO,DEBUG}
                        Log level. Default: "INFO"
  --cfg-enables [CFG_ENABLES [CFG_ENABLES ...]]
                        Enable lines in gdev.cfg files gated by `enable_if`,
                        `enable_if_any`, and `enable_if_all` functions.
                        Default: "[]"
"""
HELP_DOCKERFILE_ARGUMENTS = (
    HELP_BASE_ARGUMENTS
    + """  --base-image BASE_IMAGE
                        Base image for build. Default: "ubuntu:20.04"
  --mixins [{clion,gdb,git,nano,sshd,sudo} [{clion,gdb,git,nano,sshd,sudo} ...]]
                        Image mixins to use when creating a container. Mixins
                        provide dev tools and configuration from targets in
                        the "dev_tools/gdev/mixin" directory. Default: "[]"
"""
)
HELP_BUILD_ARGUMENTS = (
    HELP_DOCKERFILE_ARGUMENTS
    + """  --platform {amd64,arm64}
                        Platform to build upon. Default: "amd64"
  --registry REGISTRY   Registry to push images and query cached build stages.
                        Default: None
"""
)
HELP_RUN_ARGUMENTS = """  -f, --force           Force Docker to build with local changes.
  --mounts MOUNTS       <host_path>:<container_path> mounts to be created (or
                        if already created, resumed) during `docker run`.
                        Paths may be specified as relative paths. <host_path>
                        relative paths are relative to the host's current
                        working directory. <container_path> relative paths are
                        relative to the Docker container's WORKDIR (AKA the
                        build dir). Default: ""
  -p [PORTS [PORTS ...]], --ports [PORTS [PORTS ...]]
                        Ports to expose in underlying docker container.
                        Default: "[]"
"""


def get_executor():
    """
    Get the executor to use for invoking the Gdev application for testing.
    """
    return SubprocessExecutor()


def test_show_help_old_no_args():
    """
    Make sure that we can show help about the various things to do, and that it is backward
    compatible with the old Gdev output.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["--backward", "--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)
    expected_output = expected_output.replace(
        "\nGaiaPlatform build and development environment tool.\n", ""
    ).replace(",gen,", ",")

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_no_args():
    """
    Make sure that we can show help about the various things to do.
    """
    # Arrange
    executor = get_executor()
    supplied_arguments = ["--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_return_code = 0
    expected_output = """usage: gdev [-h] {build,cfg,dockerfile,push,run} ...

positional arguments:
  {build,cfg,dockerfile,push,run}

optional arguments:
  -h, --help            show this help message and exit"""
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_old_build():
    """
    Make sure that we can show help about the build task, and that it is backward compatible
    with the old Gdev output.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--backward", "--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `build` subcommand entry point.",
    )
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_build():
    """
    Make sure that we can show help about the build task in the new format.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["build", "--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_return_code = 0
    expected_output = (
        """usage: gdev build [-h] [--log-level {CRITICAL,ERROR,WARNING,INFO,DEBUG}]
                  [--cfg-enables [CFG_ENABLES [CFG_ENABLES ...]]]
                  [--base-image BASE_IMAGE]
                  [--mixins [{clion,gdb,git,nano,sshd,sudo} [{clion,gdb,git,nano,sshd,sudo} ...]]]
                  [--platform {amd64,arm64}] [--registry REGISTRY]

Build the image based on the assembled dockerfile.

"""
        + HELP_BUILD_ARGUMENTS
    )
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_old_cfg():
    """
    Make sure that we can show help about the cfg task, and that it is backward compatible
    with the old Gdev output.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["cfg", "--backward", "--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Parse gdev.cfg for build rules.",
        "Class to provide for the `cfg` subcommand entry point.",
    )
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_cfg():
    """
    Make sure that we can show help about the cfg task in the new format.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["cfg", "--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_return_code = 0
    expected_output = (
        """usage: gdev cfg [-h] [--log-level {CRITICAL,ERROR,WARNING,INFO,DEBUG}]
                [--cfg-enables [CFG_ENABLES [CFG_ENABLES ...]]]

Generate the configuration used as the basis for the dockerfile.

"""
        + HELP_BASE_ARGUMENTS
    )
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_old_dockerfile():
    """
    Make sure that we can show help about the dockerfile task, and that it is backward compatible
    with the old Gdev output.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["dockerfile", "--backward", "--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `dockerfile` subcommand entry point.",
    )
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_dockerfile():
    """
    Make sure that we can show help about the dockerfile task in the new format.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["dockerfile", "--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    #
    # Note the {indent} is used within the string as the line length was exceeded.
    expected_return_code = 0
    expected_output = (
        """usage: gdev dockerfile [-h] [--log-level {CRITICAL,ERROR,WARNING,INFO,DEBUG}]
{indent}[--cfg-enables [CFG_ENABLES [CFG_ENABLES ...]]]
{indent}[--base-image BASE_IMAGE]
{indent}[--mixins [{clion,gdb,git,nano,sshd,sudo} [{clion,gdb,git,nano,sshd,sudo} ...]]]

Assemble the dockerfile based on the generated configuration.

""".replace(
            "{indent}", "                       "
        )
        + HELP_DOCKERFILE_ARGUMENTS
    )
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_old_gen():
    """
    Make sure that we can show help about the gen task, and that it is backward compatible
    with the old Gdev output.
    """

    # Arrange
    supplied_arguments = ["gen", "--backward", "--help"]

    # Act
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)
    execute_results = InProcessResult(
        expected_return_code, io.StringIO(expected_output), io.StringIO(expected_error)
    )

    # Assert
    expected_return_code = 0
    expected_output = """usage: gdev gen [-h] {apt,env,gaia,git,pip,pre_run,run,web} ...

Internal component commands for top-level gdev commands. These should rarely
be needed.

positional arguments:
  {apt,env,gaia,git,pip,pre_run,run,web}

optional arguments:
  -h, --help            show this help message and exit"""
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_gen():
    """
    As the gen task is not callable in the new version, we do not show it as
    a viable subcommand.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["gen", "--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_return_code = 2
    expected_output = ""
    expected_error = """usage: gdev [-h] {build,cfg,dockerfile,push,run} ...
gdev: error: invalid choice: 'gen' (choose from 'build', 'cfg', 'dockerfile', 'push', 'run')"""

    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_old_push():
    """
    Make sure that we can show help about the push task, and that it is backward compatible
    with the old Gdev output.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["push", "--backward", "--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `push` subcommand entry point.",
    )
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_push():
    """
    Make sure that we can show help about the push task in the new format.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["push", "--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_return_code = 0
    expected_output = (
        """usage: gdev push [-h] [--log-level {CRITICAL,ERROR,WARNING,INFO,DEBUG}]
                 [--cfg-enables [CFG_ENABLES [CFG_ENABLES ...]]]
                 [--base-image BASE_IMAGE]
                 [--mixins [{clion,gdb,git,nano,sshd,sudo} [{clion,gdb,git,nano,sshd,sudo} ...]]]
                 [--platform {amd64,arm64}] [--registry REGISTRY]

Build the image, if required, and push the image to the image registry.

"""
        + HELP_BUILD_ARGUMENTS
    )
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_old_run():
    """
    Make sure that we can show help about the run task, and that it is backward compatible
    with the old Gdev output.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--backward", "--help"]
    (
        expected_return_code,
        expected_output,
        expected_error,
    ) = determine_old_script_behavior(supplied_arguments)

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_output = expected_output.replace(
        "Dependency(options: 'Options')",
        "Class to provide for the `run` subcommand entry point.",
    )
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_show_help_new_run():
    """
    Make sure that we can show help about the run task in the new format.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--help"]

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    expected_return_code = 0
    expected_output = (
        """usage: gdev run [-h] [--log-level {CRITICAL,ERROR,WARNING,INFO,DEBUG}]
                [--cfg-enables [CFG_ENABLES [CFG_ENABLES ...]]]
                [--base-image BASE_IMAGE]
                [--mixins [{clion,gdb,git,nano,sshd,sudo} [{clion,gdb,git,nano,sshd,sudo} ...]]]
                [--platform {amd64,arm64}] [--registry REGISTRY] [-f]
                [--mounts MOUNTS] [-p [PORTS [PORTS ...]]]
                ...

Build the image, if required, and execute a container for GaiaPlatform
development.

"""
        + HELP_POSITIONAL_PREFIX
        + HELP_BUILD_ARGUMENTS
        + HELP_RUN_ARGUMENTS
    )
    expected_error = ""
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )
