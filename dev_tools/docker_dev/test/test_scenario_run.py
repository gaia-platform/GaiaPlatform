#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
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
    determine_repository_base_directory,
)
from test.test_scenarios import (
    find_drydock_line,
    find_docker_build_line,
    construct_base_build_line,
    find_and_remove,
)


def get_executor():
    """
    Get the executor to use for invoking the Gdev application for testing.
    """
    return SubprocessExecutor()


def __find_docker_run_line(expected_output):

    run_line = find_drydock_line(expected_output, "[execvpe:docker run ")
    print(f"run_line:{run_line}")
    return run_line


def __construct_base_run_command_line(is_using_mixins=False):
    run_type = "custom" if is_using_mixins else "run"
    return (
        f"--rm --init --entrypoint /bin/bash --hostname production__{run_type} "
        + "--platform linux/amd64 "
        + f"--privileged  --volume {determine_repository_base_directory()}:"
        + f"/source production__{run_type}:latest"
    )


def test_generate_docker_run():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  Note that this uses the `--drydock` test argument to simulate
    calling docker without actually calling docker.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_mixin_clion():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The `clion` mixin adds to the build line and the run line.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--mixins", "clion"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(build_line, " --label Mixins=\"['clion']\"")
    assert build_line == construct_base_build_line(is_using_mixins=True)

    run_line = __find_docker_run_line(expected_output)
    run_line = find_and_remove(
        run_line,
        " -p 22:22 --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --user 1000:1000",
    )
    assert run_line == __construct_base_run_command_line(is_using_mixins=True)


def test_generate_docker_run_mixin_nano():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The `nano` mixin only adds to the arguments passed to
    the docker build command line.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--mixins", "nano"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(build_line, " --label Mixins=\"['nano']\"")
    assert build_line == construct_base_build_line(is_using_mixins=True)

    run_line = __find_docker_run_line(expected_output)
    assert run_line == __construct_base_run_command_line(is_using_mixins=True)


def test_generate_docker_run_mounts():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The mount command allows us to mount an extra directory
    into the container.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--mount", "/root:/host"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    run_line = find_and_remove(
        run_line,
        "--mount type=volume,dst=/host,volume-driver=local,volume-opt=type=none,"
        + "volume-opt=o=bind,volume-opt=device=/root ",
    )
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_platform():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The platform command is supported, but it is not clear if
    any of our machines can run in a cross-platform mode.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--platform", "arm64"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(
        build_line, " --platform linux/arm64", replace_with=" --platform linux/amd64"
    )
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    run_line = find_and_remove(
        run_line, " --platform linux/arm64", replace_with=" --platform linux/amd64"
    )
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_ports():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The ports argument will make sure to expose needed ports.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--ports", "1234"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    run_line = find_and_remove(run_line, " -p 1234:1234")
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_registry():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The registry argument provides a location to cache the build
    steps from.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--registry", "localhost"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    build_line = find_and_remove(
        build_line,
        "--cache-from localhost/production__",
        search_for_end_whitespace=True,
    )
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_force():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  The force argument instructs the build stage to be forced.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--force"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_args_old():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  Any trailing arguments are passed to container.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--backward", "--dry-dock", "not-an-argument"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    run_line = find_and_remove(run_line, ' -c "not-an-argument"', look_at_end=True)
    assert run_line == __construct_base_run_command_line()


def test_generate_docker_run_args_new():
    """
    Per request, only the `--` form of the argument is preserved going forward.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "not-an-argument"]
    expected_return_code = 0
    expected_output = ""
    expected_error = "arguments to pass to docker run must be prefaced with `--`"

    # Act
    execute_results = executor.invoke_main(
        arguments=supplied_arguments, cwd=determine_repository_production_directory()
    )

    # Assert
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )


def test_generate_docker_run_explicit_args():
    """
    Make sure that we can generate a request to docker to run the image built
    by previous steps.  Any trailing arguments are passed to container.
    """

    # Arrange
    executor = get_executor()
    supplied_arguments = ["run", "--dry-dock", "--", "not-an-argument"]
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
    execute_results.assert_results(
        expected_output, expected_error, expected_return_code
    )

    build_line = find_docker_build_line(expected_output)
    assert build_line == construct_base_build_line()

    run_line = __find_docker_run_line(expected_output)
    run_line = find_and_remove(run_line, ' -c "not-an-argument"', look_at_end=True)
    assert run_line == __construct_base_run_command_line()
